cppflags=-fpermissive -std=c++14 -O3

default: page

page: *.h page.cpp
	g++ $(cppflags) page.cpp -o page
	
clean:
	rm -f *.out *.s page

.PHONY: clean
