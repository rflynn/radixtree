# Radix Tree and URL Tree

## Description
A radix tree is a space-optimized trie where nodes are merged
with single child nodes, greatly reducing memory usage and
lookup time for datasets with significant overlap.

### Install
```sh
git clone git://github.com/rflynn/radixtree.git
cd radixtree
python setup.py build
python setup.py test
sudo python setup.py install
```

### Example
```python
from radixtree import RadixTree
r = RadixTree()
for url in ['http://foo', 'http://foo/bar', 'http://baz']:
    r.insert(url)
print r
```

## References
1. http://en.wikipedia.org/wiki/Radix_tree
2. http://en.wikipedia.org/wiki/Directed_acyclic_word_graph

