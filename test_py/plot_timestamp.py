#!/usr/bin/env python3
#
# usage: plot_timestamp.py hashdb.hdb
#
# graph timestamps from <hashdb.hdb>/timestamp.json into
# temp_deltas.pdf and temp_totals.pdf

import sys
import pylab
import matplotlib.pyplot as plot
from argparse import ArgumentParser
from plot_performance import read_timestamps, plot_deltas, plot_totals

if __name__=="__main__":

    parser = ArgumentParser(prog='plot_timestamp.py', description='Plot timestamps in timstamp.json file of hashdb')
    parser.add_argument('hashdb_dir', help= 'hashdb directory')
    args = parser.parse_args() 
    hashdb_dir = args.hashdb_dir

    deltas, totals = read_timestamps(hashdb_dir)
    plot_deltas(deltas, "temp_deltas.pdf")
    plot_totals(totals, "temp_totals.pdf")

