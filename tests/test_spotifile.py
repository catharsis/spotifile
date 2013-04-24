from nose import with_setup, run as noserun
from nose.tools import *
import unittest
import os, sys, time
from os import path
from subprocess import call, check_call
from sh import ls, cat


mountpoint = '/tmp/spotifile_test_mount'

class SpotifileTest(unittest.TestCase):
	def test_ls( self ):
		assert_true('connection' in ls(mountpoint))

	def test_cat_connection( self ):
		assert_equal('logged in', cat(path.join(mountpoint, 'connection')))

	@classmethod
	def teardownClass(cls):
		check_call(['fusermount', '-u', mountpoint])
		if path.exists(mountpoint):
			os.rmdir(mountpoint)

	@classmethod
	def setupClass(cls):
		if not path.exists(mountpoint):
			os.mkdir(mountpoint)
		username = input()
		check_call(['./spotifile', '-o', 'username=%s' % username, mountpoint])
		time.sleep(1) #give it some time

