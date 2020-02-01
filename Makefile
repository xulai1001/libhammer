CXXFLAGS=-O3 -fpermissive
LDFLAGS=-Llibhammer -lsigsegv -lhammer

binaries=google_rowhammer attack target revise templating

all: $(binaries)

libhammer:
	+make -C $@

$(binaries): %: %.cpp libhammer
	g++ $< $(CXXFLAGS) -o $@ $(LDFLAGS) 

clean:
	rm $(binaries)
	+make -C libhammer clean

default: all
.PHONY: clean
