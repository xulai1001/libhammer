CFLAGS=-O3 -g
CXXFLAGS=-O3 -std=c++14 -g -fpermissive
LDFLAGS=-lsigsegv

obj-cxx=addrmap.o page.o templating.o utils.o
obj-c=memory.o timing.o

default: libhammer

libhammer: $(obj-cxx) $(obj-c)
	ar -rv libhammer.a *.o
	rm *.o
	
$(obj-cxx): %.o: %.cpp
	g++ -c $(CXXFLAGS) -o $@ $< 
    
$(obj-c): %.o: %.c
	gcc -c $(CFLAGS) -o $@ $<

clean:
	rm libhammer.a

.PHONY: clean
