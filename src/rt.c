/* ex: set ts=4 et: */
/*
 *
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
    if (a->len != b->len)
        return a->len > b->len ? 1 : -1;
    return memcmp(a->s, b->s, a->len);
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

static str * str_new(const char *c, size_t len)
{
    str *s = malloc(sizeof *s);
    str_init(s, c, len);
    return s;
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
    printf("%s n=%p key=%.*s\n",
        __func__,  n, (unsigned)key->len, key->s);
    for (; n; n = n->next)
    {
        printf("%s n->key=%.*s key=%.*s strcmp(n->key, key)=%d\n",
            __func__,
            (unsigned)n->key.len, n->key.s,
            (unsigned)key->len, key->s,
            str_cmp(&n->key, key));
        if (!str_cmp(&n->key, key))
            return n;
    }
    printf("%s done.\n", __func__);
    return 0;
}

static node * node_set(node **n, str *key, node *val)
{
    if (!*n)
    {
        *n = node_new(key, val);
        return *n;
    } else {
        node *v = node_get(*n, key);
        if (!v) {
            printf("%s:%u *n=%p !v\n", __func__, __LINE__, *n);
            v = node_new(key, val);
            while (str_cmp(key, &(*n)->key) > 0)
            {
                if ((*n)->next)
                    *n = (*n)->next;
                else
                    break;
            }
            v->next = (*n)->next;
            (*n)->next = v;
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
    size_t olen = len;
    if (r->root && !r->root->key.len)
        *buf++ = '(', len--;
    len -= node_to_str(r->root, buf, len);
    if (r->root && !r->root->key.len)
        *buf++ = ')', len--;
    return olen - len;
}

int radixtree_add(radixtree *r, const char *s, size_t len)
{
    str word;
    node *curr = r->root;
    size_t consumed = 0, lp = 0;
    str_init(&word, s, len);
    printf("radixtree_insert(%.*s):\n", (unsigned)len, s);
    node_dump(r->root, stdout, 1);

    while (consumed < len)
    {
        node *prefix;
        lp = longest_prefix(&word, curr, &prefix);
        if (!prefix)
            break;
        printf("   prefix(%p).str=%.*s\n",
            prefix, (unsigned)prefix->key.len, prefix->key.s);
        if (prefix->key.len != lp)
        {
            /* split key into prefix:suffix, move data */
            str keysuf = {
                prefix->key.len - lp,
                prefix->key.s + lp
            };
            prefix->key.len = lp;
            curr = node_set(&prefix->val, &keysuf, 0);
            printf("suffix split: keysuf=%.*s\n",
                (unsigned)keysuf.len, keysuf.s);
                node_dump(r->root, stdout, 0);
        } else {
            curr = prefix;
            printf("exact match, curr=%p\n", curr);
        }
        word.s += lp, word.len -= lp;
        printf("prefix loop curr=%p\n", curr);
        consumed += lp;
    }

    printf("prefix done curr=%p word=%.*s consumed=%zu lp=%zu\n",
        curr, (unsigned)word.len, word.s, consumed, lp);
    node_dump(r->root, stdout, 1);

    if (word.len)
    {
        /* rest of unmatched string */
        curr = node_set(curr ? &curr->val : &r->root, &word, 0);
        //consumed = len;
        word.len = 0;
    }

    printf("post-word.len:\n");
    node_dump(r->root, stdout, 1);

    /* mark end of string */
    printf("eos r->root=%p curr=%p\n", r->root, curr);
    curr = node_set(
        curr ? &curr->val : &r->root,
            &word, 0);
#if 0
    curr = node_set(
        curr ? consumed < len ? &curr->val : &curr : &r->root,
            &word, 0);
#endif

    printf("done:\n");
    node_dump(r->root, stdout, 1);
    printf("curr: ");
    node_dump(curr, stdout, 1);

    assert(r->root);
    //assert(consumed == len);
    assert(curr);

    return 1;
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

static const char * test_empty_string(radixtree *t)
{
    radixtree_add(t, "", 0);
    return "()";
}

static const char * test_prefix_first(radixtree *t)
{
    radixtree_add(t, "hell", 4);
    radixtree_add(t, "hello", 5);
    return "hell(,o())";
}

static const char * test_prefix_last(radixtree *t)
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
    { TESTLIST(test_empty_string) },
    { TESTLIST(test_add)          },
    { TESTLIST(test_dupe)         },
    { TESTLIST(test_prefix_first) },
  //{ TESTLIST(test_hellheck)     },
    { TESTLIST(test_prefix_last)  },
    { TESTLIST(test_2neighbors)   },
    { TESTLIST(test_3neighbors)   },
    { TESTLIST(test_hehellhello)  },
    { TESTLIST(test_hellohellhe)  },
    { TESTLIST(test_hellhehello)  },
    { TESTLIST(test_hellhellohe)  },
  //{ TESTLIST(test_urls)         },
    { 0, 0                        },
#undef TESTLIST
};

int main(void)
{
    const struct unittest *test = Tests;
    unsigned passcnt = 0;
    while (test->func)
    {
        static char buf[1024];
        radixtree t;
        const char *expected;
        printf("\n=> %s\n", test->name);
        fflush(stdout);
        radixtree_init(&t);
        expected = test->func(&t);
        node_dump(t.root, stdout, 0);
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

