spotifile
=========

FUSE file system for Spotify

The goal of this project is to be able to provide a synthethic file system
as an interface towards Spotify. That includes, for example, being able
to check the state of your session by doing:

$ cat /home/alofgren/spotifile/connection
logged in

We also want a nice way to browse artists/albums/playlists etc. through
the file system, such as:

$ cd /home/alofgren/spotifile/artists/R/Refused/
$ ls
albums biography

and so forth.

=== LIBSPOTIFY APP KEY ===
In order to compile spotifile, you need to provide your own appkey.c.
The reason for this is that the Terms of Use for the app key puts all
liability regarding misuse of the key on the original holder.

I'm hoping that Spotify will reconsider that decision in the future, or at
least provides some means for developers to prevent misuse, other than
compiling/obfuscating the key.
