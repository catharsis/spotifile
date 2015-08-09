#!/usr/bin/python
from nose.tools import *
from tempfile import mkdtemp
from subprocess import check_call, check_output
import os, time

def eventually_eq_(a, b_op, timeout=5, tick=0.1):
    time_spent = 0
    b = b_op()
    while (time_spent < timeout) and not a == b:
        time.sleep(tick)
        time_spent = time_spent + tick
        b = b_op()

    if not a == b:
        raise AssertionError("%r != %r", a, b)

test_dir = None
def setup_module():
    global test_dir
    test_dir = mkdtemp()
    print("Mounting at %s" % test_dir)
    check_call(["./spotifile", test_dir])

def teardown_module():
    global test_dir
    print("Unmounting %s" % test_dir)
    check_call(["fusermount", "-z", "-u", test_dir])
    os.rmdir(test_dir)

def spotifile_path(spath):
    global test_dir
    if isinstance(spath, list):
        return os.path.join(test_dir, *spath)

    return os.path.join(test_dir, spath)

def logged_in():
    eventually_eq_(b'logged in',
            lambda: check_output(["cat", spotifile_path("connection")]))

@with_setup(logged_in, None)
def test_playlists():
    playlists = os.listdir(spotifile_path("playlists"))
    assert_greater(len(playlists), 0)
    for pl in playlists:
        playlist_path = spotifile_path([spotifile_path("playlists"), pl])
        tracks = os.listdir(playlist_path)
        assert_greater(len(tracks), 0)
        for t in tracks:
            track_path = spotifile_path([playlist_path, t])
            track_content = os.listdir(track_path)
            assert_greater(len(track_content), 0)
