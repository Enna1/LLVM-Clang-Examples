all: test

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O2

clean:
	rm -f *.o *~ *.so
