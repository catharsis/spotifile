[![Build Status](https://travis-ci.org/catharsis/spotifile.svg?branch=master)](https://travis-ci.org/catharsis/spotifile)
spotifile 
=========

FUSE file system for Spotify

The goal of this project is to be able to provide a synthethic file system
as an interface towards Spotify. That includes, for example, being able
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
    $ cat Long\ Gone\ Lonesome\ Blues/pcm| aplay -r 44100 -c 2 -f S16_LE                     


and so forth.

## Roadmap/Open issues
Searching is currently TBD; though I'm considering making use of ioctl's for this.
Convenience scripts/wrappers (for stuff like listing search results & dmenu/rofi interop) will most likely be needed for this to be usable on a day-to-day basis.

## Oh dear, why?
For fun, mostly. But also because I've been looking for a media playing solution that is both scriptable and ties into my otherwise somewhat minimalistic desktop environment nicely. I think this approach is not as crazy as it might initially sound for those purposes. It's worth a shot at least, yes?

## dependencies
> * libspotify
> * fuse >= 2.6
> * autotools
> * GLib2
