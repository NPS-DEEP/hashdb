#!/usr/bin/python
#
# hashdb_helpers.py
# A module for helping with hashdb tests
from subprocess import Popen, PIPE
import os
import shutil
import zipfile
import gzip

# run command array and return lines from it
def hashdb(cmd):
    # find hashdb
    if os.path.isfile("../src/hashdb"):
        # path for "make check" from base dir
        cmd.insert(0, "../src/hashdb")
    elif os.path.isfile("../src/hashdb.exe"):
        # win path for "make check" from base dir
        cmd.insert(0, "../src/hashdb.exe")
    elif os.path.isfile("./src/hashdb"):
        # path for "make check" from test dir
        cmd.insert(0, "./src/hashdb")
    elif os.path.isfile("./src/hashdb.exe"):
        # win path for "make check" from test dir
        cmd.insert(0, "./src/hashdb.exe")
    else:
        print("hashdb tool not found.  Aborting.\n")
        raise ValueError("hashdb not found")

    # run hashdb command
    p = Popen(cmd, stdout=PIPE)
    lines = p.communicate()[0].decode('utf-8').split("\n")
    if p.returncode != 0:
        print("error with command", cmd)
        print("lines", lines)
        print("Aborting.")
        raise Exception("hashdb aborted.")

    return lines

def read_file(filename):
    with open(filename, 'r') as myfile:
        lines = myfile.readlines()
        return lines

# require equality
def str_equals(a,b):
    if a.strip() != b.strip():   # strip compensates for Windows line endings
        raise ValueError(a.strip() + " not equal to " + b.strip())
def bool_equals(a,b):
    if a != b:
        raise ValueError(a + " not equal to " + b)
def int_equals(a,b):
    if a != b:
        raise ValueError(str(a) + " not equal to " + str(b))

def _bad_list(a,b):
    print("a:")
    for line in a:
        print("'%s'" % line.strip())
    print("b:")
    for line in b:
        print("'%s'" % line.strip())
    raise ValueError("list mismatch")

# length must be the same, but specific comments may differ
def lines_equals(a,b):
    # length differs
    if len(a) != len(b) or len(a) == 0:
        print("line counts differ: a: %d, b: %d" % (len(a), len(b)))
        _bad_list(a,b)
    # lines differ
    for item_a, item_b in zip(a, b):
        if item_a[:11] == '# command: ' and item_b[:11] == '# command: ':
            continue
        if item_a[:18] == '# hashdb-Version: ' and item_b[:18] == '# hashdb-Version: ':
            continue
        if item_a.strip() != item_b.strip():
            print("mismatch:\na: '" + item_a.strip() + "',\nb:'" + item_b.strip() + "'")
            _bad_list(a,b)

def rm_tempfile(filename):
    if filename[:5] != "temp_":
        # safeguard
        print("aborting, invalid filename:", filename)
        raise ValueError("%s invalid filename." % filename)
    if os.path.exists(filename):
        os.remove(filename)
    if os.path.exists(filename):
        raise ValueError("%s not removed." % filename)
 
def rm_tempdir(dirname):
    if dirname[:5] != "temp_":
        # safeguard
        print("aborting, invalid dirname:", dirname)
        raise ValueError("%s invalid dirname." % filename)
    if os.path.exists(dirname):
        # remove existing DB
        shutil.rmtree(dirname, True)
    if os.path.exists(dirname):
        raise ValueError("%s not removed." % filename)

def make_tempfile(filename, lines):
    if filename[:5] != "temp_":
        # safeguard
        print("aborting, invalid filename:", filename)
        raise ValueError("%s invalid filename." % filename)

    # overwrite file with lines
    f = open(filename, 'w')
    for line in lines:
        f.write("%s\n" % line)

# create new tempdir containing json_data
def make_hashdb(tempdir, json_data):
    rm_tempdir(tempdir)
    rm_tempfile("temp_0.json")
    hashdb(["create", tempdir])
    make_tempfile("temp_0.json", json_data)
    hashdb(["import", tempdir, "temp_0.json"])

# create new media image containing compressed content
def make_temp_media(filename):
    if filename[:5] != "temp_":
        # safeguard
        print("aborting, invalid filename:", filename)
        raise ValueError("%s invalid filename." % filename)

    if os.path.exists(filename):
        os.remove(filename)

    # put in 600 bytes of 0x00
    with open(filename, 'wb') as f:
        f.write(bytes(''.join('\0'*600), 'UTF-8'))

    # create temp_0_file_1 for zip
    with open("temp_0_file_1", 'w') as f:
        f.write("temp_0_file_1 content")

    # create temp_0_file_2 for zip
    with open("temp_0_file_2", 'w') as f:
        f.write("temp_0_file_2 content")

    # put temp_0_file_1 and temp_0_file_2 in zip, compressed
    with zipfile.ZipFile(filename, 'a', zipfile.ZIP_DEFLATED) as f:
        f.write("temp_0_file_1")
        f.write("temp_0_file_2")

    # put in gzip content
    with gzip.open(filename, 'ab') as f:
        f.write(b"gzip content")

