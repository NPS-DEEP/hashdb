#!/usr/bin/env python3
# plot timestamp graph based on command indicated in log

import os
import shutil
from subprocess import Popen, PIPE
import sys
import json
import pylab
import matplotlib.pyplot as plot

plotsize=(6,3.5)

def run_hashdb(cmd):

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
    # from http://stackoverflow.com/questions/17411966/printing-stdout-in-realtime-from-a-subprocess-that-requires-stdin
    p = Popen(cmd, stdout=PIPE)
    while p.poll() is None:
        l = p.stdout.readline() # This blocks until it receives a newline.
        print(l)
    # When the subprocess terminates there might be unconsumed output 
    # that still needs to be processed.
    print(p.stdout.read())
 
    if p.returncode != 0:
        print("Error with command: ",cmd)
        raise ValueError("hashdb aborted")

def rm_tempdir(dirname):
    if dirname[:5] != "temp_":
        # safeguard
        print("aborting, invalid dirname:", dirname)
        raise ValueError("%s not found." % dirname)
    if os.path.exists(dirname):
        # remove existing DB
        shutil.rmtree(dirname, True)

def read_timestamps(hashdb_dir):
    deltas = list()
    totals = list()
    timestamp_file = os.path.join(hashdb_dir, "timestamp.json")
    with open(timestamp_file, 'r') as infile:
        for line in infile:
            l = line.strip()
            if len(l) == 0 or l[0] == '#':
                continue

            # the line needs to be a timestamp line
            json_data = json.loads(line)
            deltas.append(json_data["delta"])
            totals.append(json_data["total"])

    print("deltas",deltas)
    print("totals",totals)
    return deltas, totals

def plot_deltas(deltas, out_pdf):
    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Hashes already present in millions")
    ax.set_ylabel("Time in seconds")
    ax.set_title("Time to process 100,000 more hashes")
    xes=[]
    for n in range(0,len(deltas)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        xes.append(n*.1)

    plot.bar(xes, deltas, width=0.08)
    fig.savefig(out_pdf)

def plot_totals(totals, out_pdf):
    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Time in seconds")
    ax.set_ylabel("Hashes processed in millions")
    ax.set_title('Time processing hashes')
    ys=[]
    for n in range(0,len(totals)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        ys.append(n*.1)

    plot.plot(totals, ys)
    fig.savefig(out_pdf)

def run_random():
    # add_random
    print("add_random...")
    rm_tempdir("temp_random.hdb")
    run_hashdb(["create", "temp_random.hdb"])
    run_hashdb(["add_random", "temp_random.hdb", "1000000"])
    deltas, totals = read_timestamps("temp_random.hdb")
    plot_deltas(deltas, "temp_add_random_delta.pdf")
    plot_totals(totals, "temp_add_random_total.pdf")

    # scan_random
    print("scan_random...")
    run_hashdb(["scan_random", "temp_random.hdb", "1000000"])
    deltas, totals = read_timestamps("temp_random.hdb")
    plot_deltas(deltas, "temp_scan_random_delta.pdf")
    plot_totals(totals, "temp_scan_random_total.pdf")

def run_same():
    # add_same
    print("add_same...")
    rm_tempdir("temp_same.hdb")
    run_hashdb(["create", "-m100", "temp_same.hdb"])
    run_hashdb(["add_same", "temp_same.hdb", "1000000"])
    deltas, totals = read_timestamps("temp_same.hdb")
    plot_deltas(deltas, "temp_add_same_delta.pdf")
    plot_totals(totals, "temp_add_same_total.pdf")

    # scan_same
    print("scan_same...")
    run_hashdb(["scan_same", "temp_same.hdb", "1000000"])
    deltas, totals = read_timestamps("temp_same.hdb")
    plot_deltas(deltas, "temp_scan_same_delta.pdf")
    plot_totals(totals, "temp_scan_same_total.pdf")

def run_key_space():
    # add_random with small key space
    print("add_random small keyspace...")
    rm_tempdir("temp_random_key.hdb")
    run_hashdb(["create", "temp_random_key.hdb", "-t9:3"])
    run_hashdb(["add_random", "temp_random_key.hdb", "1000000"])
    deltas, totals = read_timestamps("temp_random_key.hdb")
    plot_deltas(deltas, "temp_add_random_key_delta.pdf")
    plot_totals(totals, "temp_add_random_key_total.pdf")

    # scan_random with small key space
    print("scan_random small keyspace...")
    run_hashdb(["scan_random", "temp_random_key.hdb", "1000000"])
    deltas, totals = read_timestamps("temp_random_key.hdb")
    plot_deltas(deltas, "temp_scan_random_key_delta.pdf")
    plot_totals(totals, "temp_scan_random_key_total.pdf")

if __name__=="__main__":
    run_random()
    run_same()
    run_key_space()

