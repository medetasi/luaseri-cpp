luaseri.so: ./lua-seri.cpp
	g++ -g -O3 -Wall -shared -fPIC $^ -o $@ -I /usr/include/lua5.3/

.PHONY:clean

clean:
	rm *.so