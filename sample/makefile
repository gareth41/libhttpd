all: main ../lib/libhttpd.so

run: all
	LD_LIBRARY_PATH=../lib/ ./main

new: clean
	$(MAKE) --no-print-directory all

clean:
	rm -rf main

main: main.c
	gcc -Wall $^ -o $@ -I.. -L../lib -lhttpd -lpthread
