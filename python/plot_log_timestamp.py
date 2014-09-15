#!/usr/bin/env python2.7
# plot timestamp graph based on command indicated in log

import sys
import pylab
import matplotlib.pyplot as plot
import xml.etree.ElementTree as ET

try:
    from argparse import ArgumentParser
except ImportError:
    raise ImportError("This script requires ArgumentParser which is in Python 2.7 or Python 3.0")

plotsize=(6,3.5)
totals=[]
deltas=[]
scan_random_matching_totals=[]
scan_random_matching_deltas=[]


def crunch_xml():
    tree = ET.parse(filename)
    root = tree.getroot()

    # command
    command = root.find('command').get('name', 'undefined')

    # scan_random is different from other commands
    if command == 'scan_random':
        # timestamp for scan_random
        for timestamp in root.iter('timestamp'):
            if timestamp.get('name', '').find('scanned random hash')==0:
                deltas.append(float(timestamp.get('delta', '0')))
                totals.append(float(timestamp.get('total', '0')))
            if timestamp.get('name', '').find('scanned random matching hash')==0:
                scan_random_matching_totals.append(float(timestamp.get('total', '0')))
                scan_random_matching_deltas.append(float(timestamp.get('delta', '0')))
    else:
        # timestamp for everything else
        for timestamp in root.iter('timestamp'):
            if timestamp.get('name', '').find('Processing index')==0:
                deltas.append(float(timestamp.get('delta', '0')))
                totals.append(float(timestamp.get('total', '0')))
    return command

# add_random
def plot_add_random_total():

    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Time in seconds")
    ax.set_ylabel("Hashes added in millions")
    ax.set_title('Speed of importing hashes')
    ys=[]
    for n in range(0,len(totals)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        ys.append(n*.1)

    plot.plot(totals, ys)
    fig.savefig(plotname+"_add_random_total.pdf")

def plot_add_random_delta():
    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Hashes already present in millions")
    ax.set_ylabel("Time in seconds")
    ax.set_title("Time to add 100,000 more hashes")
    xes=[]
    for n in range(0,len(deltas)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        xes.append(n*.1)

    plot.bar(xes, deltas, width=0.08)
    fig.savefig(plotname+"_add_random_delta.pdf")

# scan_random
def plot_scan_random_total():
    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Time in seconds")
    ax.set_ylabel("Hashes scanned in millions")
    ax.set_title("Speed of scanning hashes, no matches")
    ys=[]
    for n in range(0,len(totals)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        ys.append(n*.1)

    plot.plot(totals, ys)
    fig.savefig(plotname+"_timestamp_random_total.pdf")

def plot_scan_random_delta():
    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Random hashes scanned in millions")
    ax.set_ylabel("Time in seconds")
    ax.set_title("Time to scan 100,000 random hashes")
    xes=[]
    for n in range(0,len(scan_random_matching_deltas)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        xes.append(n*.1)

    plot.bar(xes, deltas, width=0.08)
    fig.savefig(plotname+"_timestamp_random_delta.pdf")

def plot_scan_random_matching_total():
    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Time in seconds")
    ax.set_ylabel("Hashes scanned in millions")
    ax.set_title("Speed of scanning hashes, all match")
    ys=[]
    for n in range(0,len(scan_random_matching_totals)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        ys.append(n*.1)

    plot.plot(scan_random_matching_totals, ys)
    fig.savefig(plotname+"_timestamp_random_matching_total.pdf")

def plot_scan_random_matching_delta():
    fig = plot.figure(figsize=plotsize)
    ax = fig.add_axes([0.20, 0.2, 0.70, 0.70])
    ax.set_xlabel("Random matching hashes scanned in millions")
    ax.set_ylabel("Time in seconds")
    ax.set_title("Time to scan 100,000 random matching hashes")
    xes=[]
    for n in range(0,len(scan_random_matching_deltas)):
        # rescale from units of 100,000 hashes to units of 0.1 million hashes
        xes.append(n*.1)

    plot.bar(xes, scan_random_matching_deltas, width=0.08)
    fig.savefig(plotname+"_timestamp_random_matching_delta.pdf")

# main
if __name__=="__main__":

    parser = ArgumentParser(prog='plot_log_timestamp.py', description='Create graph plots from log.xml based on log command type')
    parser.add_argument('filename', help= 'filename of log.xml file')
    parser.add_argument('plotname', help= 'name to use for the generated plot')
    args = parser.parse_args() 
    filename = args.filename
    plotname = args.plotname

    # get command and timestamp data
    command = crunch_xml()

    # plot based on command indicated
    if command == 'add_random':
        print("graph for add_random")
        plot_add_random_total()
        plot_add_random_delta()
    elif command == 'scan_random':
        print("graph for scan_random")
        plot_scan_random_total()
        plot_scan_random_delta()
        plot_scan_random_matching_total()
        plot_scan_random_matching_delta()
    else:
        sys.exit("Error: malformed log: no command tag: '" + command + "'")

