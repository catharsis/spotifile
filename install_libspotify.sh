#!/bin/sh
set -e
SPOTIFILE_MARCH=$(SPOTIFILE_MARCH:-x86_64)
PKG_NAME="libspotify-12.1.51-Linux-$(SPOTIFILE_MARCH)-release"
wget https://developer.spotify.com/download/libspotify/$(PKG_NAME).tar.gz
tar -xvf $(PKG_NAME).tar.gz
cd $(PKG_NAME) && patch < ../libspotify-Makefile.patch && make prefix=$HOME/libspotify install

