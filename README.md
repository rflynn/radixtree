# Radix Tree and URL Tree

### Install
```sh
git clone git://github.com/rflynn/radixtree.git
cd radixtree
python setup.py build
python setup.py test
sudo python setup.py install
```

## Description
A radix tree is a space-optimized trie where nodes are merged
with single child nodes, greatly reducing memory usage and
lookup time for datasets with significant overlap.

### Example
```python
from radixtree import RadixTree
r = RadixTree()
for url in ['http://foo', 'http://foo/bar', 'http://baz']:
    r.insert(url)
print r
```

## References
http://en.wikipedia.org/wiki/Radix_tree

