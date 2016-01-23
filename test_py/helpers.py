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
        exit(1)

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

def rm_tempfile(filename):
    if filename[:5] != "temp_":
        # safeguard
        print("aborting, invalid filename:", filename)
        exit(1)
    if os.path.exists(filename):
        os.remove(filename)
 
def rm_tempdir(dirname):
    if dirname[:5] != "temp_":
        # safeguard
        print("aborting, invalid dirname:", filename)
        exit(1)
    if os.path.exists(dirname):
        # remove existing DB
        shutil.rmtree(dirname, True)

