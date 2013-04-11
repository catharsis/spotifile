OBJS = spfs_fuse.o spfs_spotify.o appkey.o spotify-fs.o 
CFLAGS += -g -Wall -Werror
CFLAGS += `pkg-config fuse libspotify --cflags`
spotifile: $(OBJS)
	gcc $(CFLAGS) -o $@  $^ `pkg-config fuse libspotify --libs`

spotify-fs.o: spotify-fs.c
	gcc $(CFLAGS) -c $^

clean:
	rm -f spotifile *.o

all: spotifile
