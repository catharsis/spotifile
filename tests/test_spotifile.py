import unittest
import os
from subprocess import check_call
from sh import ls


mountpoint = '/tmp/spotifile_test_mount'
class SpotifileTestClass(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		if not os.path.exists(mountpoint):
			os.mkdir(mountpoint)

	@classmethod
	def tearDownClass(cls):
		if os.path.exists(mountpoint):
			os.rmdir(mountpoint)

	def setUp(self):
		check_call(['./spotifile', mountpoint])

	def tearDown(self):
		check_call(['fusermount', '-u', mountpoint])

	def test_ls(self):
		assert 'connection' in ls(mountpoint)
