OBJS = spfs_fuse.o spfs_spotify.o appkey.o spotify-fs.o 
CFLAGS += -g -Wall -Werror
CFLAGS += `pkg-config --cflags "fuse >= 2.6 libspotify"` -pthread
LDFLAGS += `pkg-config --libs "fuse >= 2.6 libspotify"` -pthread
spotifile: $(OBJS)
	gcc $(CFLAGS) -o $@  $^ $(LDFLAGS)

spotify-fs.o: spotify-fs.c
	gcc $(CFLAGS) -c $^

clean:
	rm -f spotifile *.o

all: spotifile

test: spotifile
	nosetests
