[![Build Status](https://travis-ci.org/catharsis/spotifile.svg?branch=master)](https://travis-ci.org/catharsis/spotifile)
spotifile 
=========

[FUSE](http://fuse.sourceforge.net/) file system for Spotify

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
    $ mplayer Long\ Gone\ Lonesome\ Blues/track.wav                   


and so forth.

![gif](http://i.imgur.com/jP91r79.gif)

## Quick start
The easiest way to get started with **`spotifile`** is to create a mountpoint somewhere (say, `/tmp/spotifile`) and run **`spotifile`** like so `./spotifile -o username=spotify_username -o password=spotify_password /tmp/spotifile`. However, that's not recommendable since it'll leave your Spotify credentials in the open for anyone else with access to your machine. Instead, most users should opt to create a configuration file `~/.config/spotifile.conf`, containing the credentials as such;

    [spotifile]
    username=myUsername
    password=myPassword
Depending on your situation, it is likely a good idea to set as restrictive permissions on the file as possible - it does contain sensitive data after all!

    chmod 600 ~/.config/spotifile.conf

Now, you can leave out the credentials from the command line;

    ./spotifile /tmp/spotifile

If everything goes as expected, you should now be able to `cd` to `/tmp/spotifile`, and check your connection status like this;

    $ cd /tmp/spotifile
    $ cat connection
    logged in

## Roadmap/Open issues
Searching is currently TBD; though I'm considering making use of ioctl's for this.
Convenience scripts/wrappers (for stuff like listing search results & dmenu/rofi interop) will most likely be needed for this to be usable on a day-to-day basis.

Currently, the focus is to get something "dog-foodable" going. For me, that includes at the very least convenient playback (through mplayer or aplay for example) with embedded riff/wave headers to avoid the drudgery of having to specify sample rates and simple browsing of existing play lists. Once that is done, I might consider a 0.1 release in some form. 

Further milestones include seek (for audio), searching, fleshed out metadata for all objects/subsystems (artists, tracks, albums, etc.). Eventually, when things have matured a bit, I hope to also include some writable parts. For example, being able to create new playlists with mkdir and ln would be cool. 

## Oh dear, why?
For fun, mostly. But also because I've been looking for a media playing solution that is both scriptable and ties into my otherwise somewhat minimalistic desktop environment nicely. I think this approach is not as crazy as it might initially sound for those purposes. It's worth a shot at least, yes?

## dependencies
> * libspotify
> * fuse >= 2.6
> * autotools
> * GLib2
