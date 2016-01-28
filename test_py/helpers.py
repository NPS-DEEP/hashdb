#!/usr/bin/python
#
# hashdb_helpers.py
# A module for helping with hashdb tests
import xml.etree.ElementTree as ET
from subprocess import Popen, PIPE
import os
import shutil

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
        print("error with command '", end="")
        print(*cmd, sep=' ', end="':\n")
        print(*lines, sep='\n')
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
    print("a")
    for line in a:
        print("'%s'\n" % line)
    print("\nb")
    for line in b:
        print("'%s'\n" % line)
    raise ValueError("list mismatch")

# length must be the same, but comments may differ
def lines_equals(a,b):
    # length differs
    if len(a) != len(b):
        _bad_list(a,b)
    # lines differ
    for item_a, item_b in zip(a, b):
        if len(item_a) != 0 and len(item_b) != 0 and \
           item_a[0] != '#' and item_b[0] != '#' and \
                                   item_a.strip() != item_b.strip():
            print("mismatch:\na: '" + item_a + "',\nb:'" + item_b + "'")
            _bad_list(a,b)

def rm_tempfile(filename):
    if filename[:5] != "temp_":
        # safeguard
        print("aborting, invalid filename:", filename)
        raise ValueError("%s not found." % filename)
    if os.path.exists(filename):
        os.remove(filename)
 
def rm_tempdir(dirname):
    if dirname[:5] != "temp_":
        # safeguard
        print("aborting, invalid dirname:", dirname)
        raise ValueError("%s not found." % dirname)
    if os.path.exists(dirname):
        # remove existing DB
        shutil.rmtree(dirname, True)

def make_tempfile(filename, lines):
    if filename[:5] != "temp_":
        # safeguard
        print("aborting, invalid filename:", filename)
        raise ValueError("%s not found." % filename)

    # overwrite file with lines
    f = open(filename, 'w')
    for line in lines:
        f.write("%s\n" % line)

