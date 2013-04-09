spotifile: spotify-fs.o appkey.o
	gcc -g -o spotifile  appkey.o spotify-fs.o `pkg-config fuse libspotify --libs`

spotify-fs.o: spotify-fs.c
	gcc -g -Wall -Werror `pkg-config fuse libspotify --cflags` -c spotify-fs.c

appkey.o: appkey.c
	gcc -g -Wall -Werror -c appkey.c

clean:
	rm -f spotifile *.o

test-cleanup:
	@fusermount -u test

test: spotifile test-cleanup
	mkdir -p ./test
	./spotifile test
	ls -AlF | grep test && cd test && ls && cd ..

all: spotifile

.PHONY=test test-cleanup
