# ex: set ts=4 et:

# Copyright 2012 Ryan Flynn <parseerror+radixtree@gmail.com>
# Radix Tree in pure python

import copy
import unittest

class RadixTree():

    def __init__(self):
        # implement Copy-on-Write (COW) by pointing all
        # End-of-String nodes to the same place
        self.EOS = {'':None} # FIXME: this SHOULD be class-static, but it doesn't work...
        self._data = {}

    def __repr__(self):
        return 'RadixTree(%s)' % (self._data,)

    def __eq__(self, other):
        return self._data == other._data

    # TODO: add __iter__ over all strings

    def insert(self, word):
        node = self._data
        while word:
            prefix, key = self.longest_prefix(word, node.keys())
            if not prefix:
                break
            lp = len(prefix)
            if prefix != key:
                # split key into prefix:suffix, move data
                keysuf = key[lp:]
                nk = node[key]
                if id(nk) == id(self.EOS):
                    nk = copy.deepcopy(nk)
                node[prefix] = {keysuf:nk}
                del node[key]
            else:
                if id(node[prefix]) == id(self.EOS):
                    node[prefix] = copy.deepcopy(self.EOS)
            word = word[lp:]
            node = node[prefix]
        # mark end of string
        #node[word] = {'':None} if word else None
        node[word] = self.EOS if word else None
        return True

    def __getitem__(self, word):
        node = self._data
        while word:
            prefix, _ = self.longest_prefix(word, node.keys())
            if not prefix:
                return False
            node = node.get(prefix, None)
            if not node:
                return False
            word = word[len(prefix):]
        return node.has_key('')

    def remove(self, word):
        # TODO: remove should condense 2+ consecutive entries
        # with only one key
        node = self._data
        while word:
            prefix, _ = self.longest_prefix(word, node.keys())
            if not prefix:
                return False
            node = node.get(prefix, None)
            if not node:
                return False
            word = word[len(prefix):]
        try:
            del node['']
            return True
        except:
            return False

    def stats(self):
        return 'nodecnt=%s' % ('unimplemented',)

    @staticmethod
    def longest_prefix(word, candidates):
        """
        return the longest prefix match between word and any of the
        candidates, if any.
        bonus: given the properties of our trie there can only be a
        single candidate with any shared prefix.
        """
        if word:
            wc = word[0]
            for c in candidates:
                if c.startswith(wc):
                    for i in reversed(xrange(1, min(len(word), len(c))+1)):
                        if c.startswith(word[:i]):
                            return (word[:i], c)
        return ('', None)

    @staticmethod
    def test():
        suite = unittest.TestLoader().loadTestsFromTestCase(RadixTreeTests)
        unittest.TextTestRunner(verbosity=2).run(suite)

class RadixTreeTests(unittest.TestCase):

    def test_00_prefixes(self):
        self.assertEqual(
            RadixTree.longest_prefix('', []),
            ('', None))
        self.assertEqual(
            RadixTree.longest_prefix('x', ['y']),
            ('', None))
        self.assertEqual(
            RadixTree.longest_prefix('x', ['x','y']),
            ('x', 'x'))
        self.assertEqual(
            RadixTree.longest_prefix('x', ['xy']),
            ('x', 'xy'))

    def test_01_new(self):
        r = RadixTree()

    def test_10_in_nop(self):
        r = RadixTree()
        self.assertFalse(r[''])

    def test_20_insert_1(self):
        r = RadixTree()
        r.insert('hell')
        self.assertTrue(r['hell'])
        self.assertFalse(r['h'])
        self.assertFalse(r[''])

    def test_21_insert_dupe(self):
        r = RadixTree()
        r.insert('hell')
        r.insert('hell')
        self.assertTrue(r['hell'])

    def test_22_insert_overlapped(self):
        r = RadixTree()
        r.insert('hell')
        r.insert('hello')
        self.assertTrue(r['hell'])
        self.assertTrue(r['hello'])

    def test_23_insert_urls(self):
        r = RadixTree()
        r.insert('http://foo')
        r.insert('http://foo.bar')
        r.insert('http://baz')

    def test_30_remove_nop(self):
        r = RadixTree()
        self.assertFalse(r.remove(''))

    def test_31_remove_1(self):
        r = RadixTree()
        r.insert('hello')
        self.assertFalse(r.remove('hell'))
        self.assertTrue(r.remove('hello'))
        self.assertFalse(r['hello'])

    def test_32_remove_overlapped(self):
        r = RadixTree()
        r.insert('hell')
        r.insert('hello')
        self.assertFalse(r['h'])
        self.assertTrue(r['hell'])
        self.assertTrue(r['hello'])
        self.assertTrue(r.remove('hell'))
        self.assertFalse(r['hell'])
        self.assertTrue(r['hello'])
        self.assertFalse(r['h'])

    def test_40_order_insensitive(self):
        r = RadixTree()
        r2 = RadixTree()
        urls = ['http://baz', 'http://foo.com', 'http://foo.com/bar']
        for url in urls:
            r.insert(url)
        # shuffle order
        urls.insert(0, urls.pop())
        for url in urls:
            r2.insert(url)
        # order shouldn't affect structure
        self.assertEqual(r, r2)

from urlparse import urlparse, urlunsplit

class URLTree(RadixTree):
    """
    Unfortunately DNS is designed backwards.
    While everything else in a URI/URL increases in specificity
    from left-to-right, domains are the opposite.
    This means that subbdomains of the same site don't share
    prefixes:
        http://foo.subdomain.example.com/
        http://bar.subdomain.example.com/
            -> {{http://},
                {foo.subdomain.example.com/,
                 bar.subdomain.example.com/}}
    This ruins our RadixTree because we end up with 2 separate entries
    for '.subdomain.example.com' when we want 1.
    This class parses mangles URL domains in reverse order under the covers
    to improve their performance in a RadixTree
        http://com.example.subdomain.foo/
        http://com.example.subdomain.bar/
            -> {{http://com.example.subdomain.},
                {foo/,bar/}}
    NOTE: RadixTree could be smarter like a DAWG and merge identical
    subtrees which would reduce memory usage, but it doesn't yet.
    """

    def __repr__(self):
        return 'URLTree(%s)' % (self._data,)

    def insert(self, url):
        url2 = self.normalize_url(url)
        return RadixTree.insert(self, url2)

    def __getitem__(self, url):
        url2 = self.normalize_url(url)
        return RadixTree.__getitem__(self, url2)

    def remove(self, url):
        url2 = self.normalize_url(url)
        return RadixTree.remove(self, url2)

    @staticmethod
    def normalize_url(url):
        u = urlparse(url)
        host = u.netloc
        userpass = ''
        port = ''
        try:
            userpass, host = host.split('@')
        except ValueError: # no '@'
            pass
        try:
            host, port = host.split(':')
        except ValueError: # no ':'
            pass
        netloc2 = URLTree.normalize_domain(host)
        if userpass:
            netloc2 = userpass + '@' + netloc2
        if port:
            netloc2 += ':' + port
        url2 = urlunsplit((u.scheme, netloc2, u.path, u.query, u.fragment))
        return url2

    @staticmethod
    def normalize_domain(domain):
        if domain.find(':') != -1:
            # IPv6 address, don't touch
            return domain
        d = domain.split('.')
        try:
            if int(d[-1]) >= 0:
                # IPv4 address, don't touch
                return domain
        except ValueError: # int() blew up
            pass
        return '.'.join(reversed(d))

    @staticmethod
    def test():
        """
        run unit tests
        """
        suite = unittest.TestLoader().loadTestsFromTestCase(URLTreeTests)
        unittest.TextTestRunner(verbosity=2).run(suite)

class URLTreeTests(unittest.TestCase):

    def test_domain_00_basecase(self):
        self.assertEqual(
            URLTree.normalize_domain(''),
            '')

    def test_domain_01_hostname(self):
        self.assertEqual(
            URLTree.normalize_domain('foo'),
            'foo')
        self.assertEqual(
            URLTree.normalize_domain('example.com'),
            'com.example')
        self.assertEqual(
            URLTree.normalize_domain('subdomain.example.com'),
            'com.example.subdomain')

    def test_domain_02_ipv4(self):
        self.assertEqual(
            URLTree.normalize_domain('0.0.0.0'),
            '0.0.0.0')
        self.assertEqual(
            URLTree.normalize_domain('255.255.255.255'),
            '255.255.255.255')
        self.assertEqual(
            URLTree.normalize_domain('127.0.0.1'),
            '127.0.0.1')

    def test_domain_03_ipv6(self):
        self.assertEqual(
            URLTree.normalize_domain('::192.168.1.1'),
            '::192.168.1.1')
        self.assertEqual(
            URLTree.normalize_domain('2001:0db8:85a3:0000:0000:8a2e:0370:7334'),
            '2001:0db8:85a3:0000:0000:8a2e:0370:7334')
        self.assertEqual(
            URLTree.normalize_domain('2001:0db8:85a3:0000:0000:8a2e:0370:0000'),
            '2001:0db8:85a3:0000:0000:8a2e:0370:0000')
        self.assertEqual(
            URLTree.normalize_domain('2001:0db8:85a3:0000:0000:8a2e:0370:aaaa'),
            '2001:0db8:85a3:0000:0000:8a2e:0370:aaaa')

    def test_url_00_basecase(self):
        self.assertEqual(URLTree.normalize_url(''), '')
        self.assertEqual(
            URLTree.normalize_url('http://'),
            'http://')

    def test_url_01_domain(self):
        self.assertEqual(
            URLTree.normalize_url('http://example.com/'),
            'http://com.example/')

    def test_url_02_port(self):
        self.assertEqual(
            URLTree.normalize_url('http://example.com:80/'),
            'http://com.example:80/')
        self.assertEqual(
            URLTree.normalize_url('https://example.com:443/'),
            'https://com.example:443/')
        self.assertEqual(
            URLTree.normalize_url('http://user:pass@example.com:80/'),
            'http://user:pass@com.example:80/')

    def test_url_03_path(self):
        self.assertEqual(
            URLTree.normalize_url('http://user:pass@example.com:80/path'),
            'http://user:pass@com.example:80/path')
        self.assertEqual(
            URLTree.normalize_url('http://user:pass@example.com:80/path/'),
            'http://user:pass@com.example:80/path/')

    def test_url_04_fragment(self):
        self.assertEqual(
            URLTree.normalize_url('http://user:pass@example.com:80/path/#anchor'),
            'http://user:pass@com.example:80/path/#anchor')

    def test_url_05_query(self):
        self.assertEqual(
            URLTree.normalize_url('http://user:pass@example.com:80/path/?k=v#anchor'),
            'http://user:pass@com.example:80/path/?k=v#anchor')
        self.assertEqual(
            URLTree.normalize_url('http://user:pass@example.com:80/path/?k=v&k2=v2#anchor'),
            'http://user:pass@com.example:80/path/?k=v&k2=v2#anchor')

