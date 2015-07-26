#!/bin/sh
wget https://developer.spotify.com/download/libspotify/libspotify-12.1.51-Linux-x86_64-release.tar.gz
tar -xvf libspotify-12.1.51-Linux-x86_64-release.tar.gz
cd libspotify-12.1.51-Linux-x86_64-release && patch < ../libspotify-Makefile.patch && make prefix=$HOME/libspotify install

