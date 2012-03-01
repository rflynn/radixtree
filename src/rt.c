/* ex: set ts=4 et: */
/*
 * Radix Tree
 *
 * Compress in-memory representation of strings sharing common prefix
 * and content.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct
{
    size_t len;
    const char *s;
} str;

struct node
{
    str key;
    struct node *val;
    struct node *next;
};
typedef struct node node;

typedef struct
{
    node *root;
} radixtree;

#define min(x,y) ((x) < (y) ? (x) : (y))

static int str_cmp(const str *a, const str *b)
{
    int cmp = memcmp(a->s, b->s, min(a->len, b->len));
    if (cmp)
        return cmp;
    return (a->len > b->len) - (a->len < b->len);
}

/*
 * return the length of shared prefix
 */
static size_t str_prefix(const str *a, const str *b)
{
    const char  *ra = a->s,
                *rb = b->s;
    size_t        l = min(a->len, b->len);
    const char *end = ra + l;
    while (ra != end && *ra == *rb)
    {
        ra++, rb++;
    }
    return (size_t)(ra - a->s);
}

static size_t longest_prefix(
    const str *word,
    node *candidates,
    node **longest)
{
    //printf("%s word=%.*s candidates=%p\n",
        //__func__, (unsigned)word->len, word->s, candidates);
    if (word->len)
    {
        const char wc = *word->s;
        for (node *c = candidates; c; c = c->next)
        {
            if (c->key.len && *c->key.s == wc)
            {
                *longest = c;
                return str_prefix(word, &c->key);
            }
        }
    }
    *longest = 0;
    return 0;
}

static void str_init(str *s, const char *c, size_t len)
{
    s->s = malloc(len);
    memcpy((char *)s->s, c, len);
    s->len = len;
}

static void node_init(node *n, str *key, node *val)
{
    n->key.s = malloc(key->len);
    memcpy((char *)n->key.s, key->s, key->len);
    n->key.len = key->len;
    n->val = val;
    n->next = 0;
}

static node * node_new(str *key, node *val)
{
    node *n = malloc(sizeof *n);
    node_init(n, key, val);
    return n;
}

static void node_free(node *n)
{
    free(n);
}

static node * node_get(node *n, const str *key)
{
    for (; n; n = n->next)
    {
        if (!str_cmp(&n->key, key))
            return n;
    }
    return 0;
}

/*
 * insert a new node in (*n)->next chain in ascending order.
 */
static node * node_set(node **n, str *key, node *val)
{
    if (!*n)
    {
        /* zero elements */
        *n = node_new(key, val);
        return *n;
    } else {
        /* 1+ elements */
        node *v = node_get(*n, key);
        if (!v) {
            node *prev = 0;
            int cmp;
            v = node_new(key, val);
            while ((cmp = str_cmp(key, &(*n)->key)) > 0)
            {
                if ((*n)->next)
                    prev = *n, *n = (*n)->next;
                else
                    break;
            }
            if (prev) {
                /* v > 1+ elements; attach after 'prev' */
                v->next = prev->next, prev->next = v;
            } else if (cmp > 0) {
                /* v > first element */
                v->next = (*n)->next, (*n)->next = v;
            } else {
                /* v < first element; copy/swap */
                node tmp = **n;
                **n = *v;
                *v = tmp;
                (*n)->next = v;
                return *n;
            }
        }
        return v;
    }
}

static void node_del(node **n, const str *key)
{
    if (*n)
    {
        node *prev = 0;
        while (*n)
        {
            if (!str_cmp(&(*n)->key, key))
            {
                node *tmp = *n;
                if (!prev)
                {
                    *n = (*n)->next;
                } else {
                    prev->next = (*n)->next;
                }
                node_free(tmp);
            }
            prev = *n;
            *n = (*n)->next;
        }
    }
}

static size_t node_to_str(const node *n, char *buf, size_t len)
{
    size_t olen = len;
    while (n)
    {
        if (0 && !n->key.len) {
            *buf++ = '(', len--;
            *buf++ = ')', len--;
            *buf = '\0';
        } else {
            int used = snprintf(buf, len, "%.*s",
                (unsigned)n->key.len, n->key.s);
            if (used > 0)
                buf += used, len -= used;
            if (n->val)
            {
                size_t consumed;
                *buf++ = '(', len--;
                consumed = node_to_str(n->val, buf, len);
                buf += consumed, len -= consumed;
                *buf++ = ')', len--;
                *buf = '\0';
            }
        }
        n = n->next;
        if (n) {
            *buf++ = ',', len--;
            *buf = '\0';
        }
    }
    return olen - len;
}

static void node_dump(const node *n, FILE *f, unsigned indent)
{
    static const char Indent[] = "                                          ";
    fprintf(f, "%.*snode(%p)", indent, Indent, n);
    if (n)
        fprintf(f, " str=\"%.*s\" next=%p val=%p",
            (unsigned)n->key.len, n->key.s, n->next, n->val);
    fputc('\n', f);
    if (n) {
        if (n->next)
            node_dump(n->next, f, indent);
        if (n->val)
            node_dump(n->val, f, indent+1);
    }
}

static void node_destroy(node *n)
{
    if (n->next)
        node_destroy(n->next);
    if (n->val)
        node_destroy(n->val);
    free(n);
}

int radixtree_init(radixtree *r)
{
    r->root = 0;
    return 1;
}

void radixtree_fini(radixtree *r)
{
    if (r->root)
        node_destroy(r->root);
    r->root = 0;
}

size_t radixtree_to_str(const radixtree *r, char *buf, size_t len)
{
    return node_to_str(r->root, buf, len);
}

/*
 * add string s[0..len] to radixtree
 * match string prefix against existing nodes.
 * at each level 'curr' if we find a 1+ char shared prefix between any
 * existing node and the remaining s[..], split that node and consume
 * that prefix from s[..]
 */
int radixtree_add(radixtree *r, const char *s, size_t len)
{
    str word;
    node *curr = r->root;

    str_init(&word, s, len);

    while (word.len)
    {
        node *prefix;
        size_t lp;

        lp = longest_prefix(&word, curr, &prefix);
        if (!prefix)
            break;
        if (prefix->key.len != lp)
        {
            /* split curr into prefix:suffix nodes */
            str keysuf = {
                prefix->key.len - lp,
                prefix->key.s + lp
            };
            node *v = prefix->val;
            prefix->val = 0;
            curr = node_set(&prefix->val, &keysuf, v);
            prefix->key.len = lp;                       
        }
        word.s += lp, word.len -= lp;
        curr = prefix->val;
    }

    /* umatched leftovers */
    curr = node_set(&curr, &word, 0);

    if (!r->root)
        r->root = curr;

    /* if leftover != end of string, add EOS */
    if (word.len)
    {
        word.len = 0;
        curr = node_set(&curr->val, &word, 0);
    }

    if (!r->root)
        r->root = curr;

    return 1;
}

/*
 * once a radixtree is built, if one will not do any further additions there
 * are some performance improvements that can be made:
 *
 * * reduce memory by replacing empty end-of-string nodes with no ->next or
 *   ->child links with a pointer to a single EOS node
 * * improve locality of reference by moving adjacent nodes to adjacent memory
 * * we could go even farther and convert each level to an array; replacing N
 *   ->next pointers with a single count, though naturally we'd need a separate
 *   search method
 *
 */
static void radixtree_finalize(radixtree *r)
{
    /* TODO */
    r = r;
}


/* * * tests * * */

static const char * test_empty_string(radixtree *t)
{
    radixtree_add(t, "", 0);
    return "";
}

static const char * test_add(radixtree *t)
{
    radixtree_add(t, "hello", 5);
    return "hello()";
}

static const char * test_dupe(radixtree *t)
{
    radixtree_add(t, "hello", 5);
    radixtree_add(t, "hello", 5);
    return "hello()";
}

static const char * test_hellhello(radixtree *t)
{
    radixtree_add(t, "hell", 4);
    radixtree_add(t, "hello", 5);
    return "hell(,o())";
}

static const char * test_hellohell(radixtree *t)
{
    radixtree_add(t, "hello", 5);
    radixtree_add(t, "hell", 4);
    return "hell(,o())";
}

static const char * test_2neighbors(radixtree *t)
{
    radixtree_add(t, "a", 1);
    radixtree_add(t, "b", 1);
    return "a(),b()";
}

static const char * test_3neighbors(radixtree *t)
{
    radixtree_add(t, "a", 1);
    radixtree_add(t, "c", 1);
    radixtree_add(t, "b", 1);
    return "a(),b(),c()";
}

static const char * test_hehellhello(radixtree *t)
{
    radixtree_add(t, "he", 2);
    radixtree_add(t, "hell", 4);
    radixtree_add(t, "hello", 5);
    return "he(,ll(,o()))";
}

static const char * test_hellohellhe(radixtree *t)
{
    radixtree_add(t, "hello", 5);
    radixtree_add(t, "hell", 4);
    radixtree_add(t, "he", 2);
    return "he(,ll(,o()))";
}

static const char * test_hellhehello(radixtree *t)
{
    radixtree_add(t, "hell", 4);
    radixtree_add(t, "he", 2);
    radixtree_add(t, "hello", 5);
    return "he(,ll(,o()))";
}

static const char * test_hellhellohe(radixtree *t)
{
    radixtree_add(t, "hell", 4);
    radixtree_add(t, "hello", 5);
    radixtree_add(t, "he", 2);
    return "he(,ll(,o()))";
}

static const char * test_hellheck(radixtree *t)
{
    radixtree_add(t, "hell", 4);
    radixtree_add(t, "heck", 4);
    return "he(ck(),ll())";
}

static const char * test_urls(radixtree *t)
{
    static const char *urls[] = {
        "http://foo",
        "http://foo/bar",
        "http://baz",
        0
    }, **url = urls;
    for (; *url; url++)
        radixtree_add(t, *url, strlen(*url));
    return "http://(baz(),foo(,/bar()))";
}

static const struct unittest {
    const char *name;
    const char * (*func)(radixtree *);
} Tests[] = {
#define TESTLIST(f) #f, f
    { TESTLIST(test_hellheck)     },
    { TESTLIST(test_empty_string) },
    { TESTLIST(test_add)          },
    { TESTLIST(test_dupe)         },
    { TESTLIST(test_hellhello)    },
    { TESTLIST(test_hellohell)    },
    { TESTLIST(test_2neighbors)   },
    { TESTLIST(test_3neighbors)   },
    { TESTLIST(test_hehellhello)  },
    { TESTLIST(test_hellohellhe)  },
    { TESTLIST(test_hellhehello)  },
    { TESTLIST(test_hellhellohe)  },
    { TESTLIST(test_urls)         },
    { 0, 0                        },
#undef TESTLIST
};

int main(void)
{
    const struct unittest *test = Tests;
    unsigned passcnt = 0;
    printf("sizeof(str)=%zu sizeof(node)=%zu\n",
        sizeof(str), sizeof(node));
    while (test->func)
    {
        static char buf[1024];
        radixtree t;
        const char *expected;
        printf("%s ", test->name);
        fflush(stdout);
        radixtree_init(&t);
        expected = test->func(&t);
        //printf("after test:\n"); node_dump(t.root, stdout, 0);
        radixtree_to_str(&t, buf, sizeof buf);
        if (!strcmp(buf, expected)) {
            puts("ok");
            passcnt++;
        } else {
            printf("!!!!!!!!!!! expected:%s got:%s\n", expected, buf);
        }
        radixtree_fini(&t);
        test++;
    }
    printf("Passed %u/%zu\n", passcnt, test - Tests);
    return 0;
}

