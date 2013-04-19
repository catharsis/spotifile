from nose import with_setup
import os
from os import path
from subprocess import check_call
from sh import ls, cat


mountpoint = '/tmp/spotifile_test_mount'

def fs_mount():
	if not path.exists(mountpoint):
		os.mkdir(mountpoint)
	check_call(['./spotifile', mountpoint])

def fs_unmount():
	check_call(['fusermount', '-u', mountpoint])
	if path.exists(mountpoint):
		os.rmdir(mountpoint)

@with_setup(fs_mount, fs_unmount)
def test_ls():
	assert 'connection' in ls(mountpoint)

@with_setup(fs_mount, fs_unmount)
def test_cat_connection():
	assert 'logged in' in cat(path.join(mountpoint, 'connection'))

