CXX ?= g++

LIBSMB_BASE ?= .

all: $(patsubst %.cxx, %, $(wildcard *.cxx))

% : %.cxx Makefile
	${CXX} -std=c++11 ${FLAGS} -static-libgcc -I ${LIBSMB_BASE}/include -L ${LIBSMB_BASE}/lib $< -static -lsmb2 -lc -o $@

clean: $(patsubst %.cxx, %.clean, $(wildcard *.cxx))

%.clean:
	-@rm $(@:.clean=)

.PHONY: clean
