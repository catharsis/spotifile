spotifile [![Build Status](https://travis-ci.org/catharsis/spotifile.svg?branch=master)](https://travis-ci.org/catharsis/spotifile) 
=========

[FUSE](http://fuse.sourceforge.net/) file system for [Spotify](https://www.spotify.com)

The goal of this project is to be able to provide a synthethic file system
as an interface towards [Spotify](https://www.spotify.com). That includes, for example, being able
to check the state of your session by doing:

    $ cat /home/alofgren/spotifile/connection
    logged in

Browsing!

    $ cd /home/alofgren/spotifile/playlists/That\ Handsome\ Devil
    $ ls
    lrwxrwxrwx 0 alofgren root 56 Jul 23 13:45 $=♡ -> ../../browse/tracks/spotify:track:3idPftQBuIvi0Mbpz7UUcc
    lrwxrwxrwx 0 alofgren root 56 Jul 23 13:45 70's Tuxedos -> ../../browse/tracks/spotify:track:275q2JSSckOAvPFF22ivc3
    lrwxrwxrwx 0 alofgren root 56 Jul 23 13:45 Adapt -> ../../browse/tracks/spotify:track:0FR8IORfrowXozk4AmN210
    lrwxrwxrwx 0 alofgren root 56 Jul 23 13:45 A Drink to Death -> ../../browse/tracks/spotify:track:1RcVweLcA8SjfJlJH3tR2K
    lrwxrwxrwx 0 alofgren root 56 Jul 23 13:45 Becky's New Car -> ../../browse/tracks/spotify:track:3Y22h4qDSUQtrqB1VD6VEC
    lrwxrwxrwx 0 alofgren root 56 Jul 23 13:45 Bored -> ../../browse/tracks/spotify:track:0PncakcV6gutcv4ps2MBK1
    lrwxrwxrwx 0 alofgren root 56 Jul 23 13:45 Bullet Math -> ../../browse/tracks/spotify:track:20J7iJSATwrvRQR3enxFN3
    ...
    
Playback!

    $ cd /home/alofgren/spotifile/playlists/Hank\ Williams
    $ mplayer Long\ Gone\ Lonesome\ Blues/track.wav


and so forth.

![gif](http://i.imgur.com/jP91r79.gif)
## Installation

### Arch Linux
A PKGBUILD for the latest released version is available in the [AUR](https://aur.archlinux.org/packages/spotifile/).

### Installing from source
To install **spotifile** from source, do the following
```Shell
$ git clone git@github.com:catharsis/spotifile.git spotifile && cd spotifile
$ autoreconf -si
$ ./configure && make && make check && make install
```
Make sure you have all the required dependencies installed, or the ./configure step will likely complain loudly.

## Quick start
The easiest way to get started with **spotifile** is to create a mountpoint somewhere (say, `/tmp/spotifile`) and run it like so `./spotifile -o username=spotify_username -o password=spotify_password /tmp/spotifile`. However, that's not recommendable since it'll leave your [Spotify](https://www.spotify.com) credentials in the open for anyone else with access to your machine. Instead, most users should opt to create a configuration file `~/.config/spotifile/spotifile.conf`, containing the credentials as such;

    [spotifile]
    username=myUsername
    password=myPassword
Depending on your situation, it is likely a good idea to set as restrictive permissions on the file as possible - it does contain sensitive data after all!

    chmod 600 ~/.config/spotifile/spotifile.conf

Now, you can leave out the credentials from the command line;

    ./spotifile /tmp/spotifile

If everything goes as expected, you should now be able to `cd` to `/tmp/spotifile`, and check your connection status like this;

    $ cd /tmp/spotifile
    $ cat connection
    logged in

To unmount the **spotifile**, simply run `fusermount -u -z /tmp/spotifile`.

## Usage
Before all else, let's consider the directory structure of a running **spotifile** instance, and briefly discuss its contents:
    
    $ ls -AlF
    total 0
    dr-x------ 0 alofgren root 0 Aug  7 09:59 browse/
    dr-x------ 0 alofgren root 0 Aug  7 09:59 playlists/
    -r--r----- 0 alofgren root 0 Aug  7 09:59 connection


In its current state, the root directory is pretty simple. It contains two directories, `browse` and `playlists`, as well as a file `connection` which contains a textual description of the current state of your [Spotify](https://www.spotify.com) connection. The main entrypoint in **spotifile** to your [Spotify](https://www.spotify.com) data is through the `playlists` directory. As you might expect, listing its contents will show you your familiar playlists. For me, that looks something like:

    $ ls -AlF playlists
    total 0
    dr-x------ 0 alofgren root 0 Aug  7 10:00 .357 String Band /
    dr-x------ 0 alofgren root 0 Aug  7 10:00 3 Inches of Blood/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Alaskan Fishermen – Alaskan Fishermen/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Antic Clay/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Arsonists/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 At The Gates — Slaughter Of The Soul/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Bathory/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Behemoth/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Blandat/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Blaze Foley/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Brown Bird/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Burzum — Filosofem/
    dr-x------ 0 alofgren root 0 Aug  7 10:00 Candlemass - Epicus Doomicus Metallicus/
    # ... snip ...

Each of these directories in turn, contain a collection of symlinks - one for each track in the playlist - pointing to the respective `track` directories in the `browse` directory. For example:

    $ ls -AlF playlists/Arsonists/
    total 0
    lrwxrwxrwx 0 alofgren root 56 Jun 20  2011 Alive -> ../../browse/tracks/spotify:track:5LEz8sfYIneF9la9icV9G0/
    lrwxrwxrwx 0 alofgren root 56 Jun 20  2011 Backdraft -> ../../browse/tracks/spotify:track:1iqvZV4BcMpV81WVJBuBTw/
    lrwxrwxrwx 0 alofgren root 56 Jun 20  2011 Blaze -> ../../browse/tracks/spotify:track:247py70aNT1jbDmnZGj3wL/
    # ... snip ...

The targets of these links are created *lazily*, meaning that they are materialized only as something referring to them is inspected by the user (such as the symlinks above). As you might've guessed (or already observed, if you've strayed from the beaten path of this guide), this means that in a newly started **spotifile** instance, the `browse` directory only contains three empty directories:

    $ tree browse
    browse
    ├── albums
    ├── artists
    └── tracks
    
    3 directories, 0 files

This pattern of lazy creation is repeated throughout **spotifile**, for all main "objects" (currently artists, albums, and tracks). Speaking of which, let's inspect one of these browse directories:

    $ $ ls -AlF browse/tracks/spotify:track:0xAyc8r3C1BNQzzDMKbOkw
    total 0
    dr-x------ 0 alofgren root 0 Aug  7 10:19 artists/
    -r--r----- 0 alofgren root 0 Aug  7 10:19 autolinked
    -r--r----- 0 alofgren root 0 Aug  7 10:19 disc
    -r--r----- 0 alofgren root 0 Aug  7 10:19 duration
    -r--r----- 0 alofgren root 0 Aug  7 10:19 index
    -r--r----- 0 alofgren root 0 Aug  7 10:19 local
    -r--r----- 0 alofgren root 0 Aug  7 10:19 name
    -r--r----- 0 alofgren root 0 Aug  7 10:19 offlinestatus
    -r--r----- 0 alofgren root 0 Aug  7 10:19 popularity
    -r--r----- 0 alofgren root 0 Aug  7 10:19 starred
    -r--r----- 0 alofgren root 0 Aug  7 10:19 track.wav

First and foremost, the name of the directory is a [URI](https://en.wikipedia.org/wiki/Uniform_resource_identifier), which uniquely identifies the resource the directory contains. If you've ever shared tracks with other [Spotify](https://www.spotify.com) users, you've likely seen one before. Most of the files in the directory are (or should be) self-explanatory. The `duration` file, for example, contains the number of milliseconds the track lasts, and the contents of the `starred` track indicate whether the track is "starred" or not.

The `track.wav` file is the actual music (if it's a music track we're looking at, and not an audiobook, or something), and should be playable by most of the music players on your system.

The `artists` directory contains links to all the artists performing on this track:

    ls -AlF browse/tracks/spotify:track:0xAyc8r3C1BNQzzDMKbOkw/artists
    total 0
    lrwxrwxrwx 0 alofgren root 54 Aug  7 10:28 Arsonists -> ../../../artists/spotify:artist:4VQ9fD75w7JlKZDIZKtpdf/

Looking inside of that directory, we see more information on the artist:

    ls -AlF browse/tracks/spotify:track:0xAyc8r3C1BNQzzDMKbOkw/artists/Arsonists/
    total 0
    dr-x------ 0 alofgren root 0 Aug  7 10:28 albums/
    dr-x------ 0 alofgren root 0 Aug  7 10:28 portraits/
    -r--r----- 0 alofgren root 0 Aug  7 10:28 biography
    -r--r----- 0 alofgren root 0 Aug  7 10:28 name

And that's the whole tour for now! 

Feel free to explore for yourself, and feel free to provide feedback, report issues, or send patches/PRs!

## Oh dear, why?
For fun, mostly. But also because I've been looking for a media playing solution that is both scriptable and ties into my otherwise somewhat minimalistic desktop environment nicely. I think this approach is not as crazy as it might initially sound for those purposes. It's worth a shot at least, yes?

## dependencies
> * libspotify
> * fuse >= 2.6
> * autotools
> * GLib2
