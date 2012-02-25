# ex: set ts=4 et:

from radixtree import RadixTree, URLTree

r = RadixTree()

r.insert('foo.bar')
r.insert('foo')
r.insert('baz')

print r

