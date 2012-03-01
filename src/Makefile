PYTHON=python
CFLAGS=-W -Wall -Wextra -std=c99 -g -O0 -m32
LDFLAGS=-m32

rt: rt.o

test: FORCE
	$(PYTHON) test.py

clean: FORCE
	$(RM) *.o rt

FORCE:
