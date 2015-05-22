#!/usr/bin/env python3
# plot graph where features are found

from argparse import ArgumentParser
import math
import xml.dom.minidom
import os
import json
import numpy
import matplotlib.pyplot
import matplotlib.cm

#import sys
#import pylab
#import matplotlib.pyplot
#import xml.etree.ElementTree as ET

block_size = 0
IMAGE_SIZE = 100*100
bulk_extractor_dir=""
image_size=0
data=numpy.zeros(IMAGE_SIZE)

def get_image_size():
    report_file = os.path.join(bulk_extractor_dir, "report.xml")
    if not os.path.exists(report_file):
        print("%s does not exist" % report_file)
        exit(1)
    xmldoc = xml.dom.minidom.parse(open(report_file, 'r'))
    image_size = int((xmldoc.getElementsByTagName("image_size")[0].firstChild.wholeText))
    return image_size

def set_data(data):
    identified_blocks_file = os.path.join(bulk_extractor_dir, "identified_blocks.txt")
    if not os.path.exists(identified_blocks_file):
        print("%s does not exist" % identified_blocks_file)
        exit(1)

    # read each line
    with open(identified_blocks_file, 'r') as f:
        for line in f:
            try:
                if line[0]=='#' or len(line)==0:
                    continue

                # get line parts
                (disk_offset,sector_hash,meta) = line.split("\t")

                # get data index
                data_index = int(int(disk_offset) * rescaler)

                # get data value
                meta = json.loads(meta)
                count = int(meta['count'])
                data_value = 1/count

                #print("disk_offset %s" % disk_offset)
                #print("data_index %s" % data_index)
                #print("data_value %s" % data_value)

                # set data value at index
                data[data_index] = data_value

            except ValueError:
                continue


# main
if __name__=="__main__":

    parser = ArgumentParser(prog='plot_rank.py', description='Plot media image grid showing matches')
    parser.add_argument('-be_dir', help= 'path to the bulk_extractor directory' , default= '/home/bdallen/demo8/temp2')
    parser.add_argument('-block_size', help= 'Block size in bytes' , default= 4096)
    args = parser.parse_args() 
    bulk_extractor_dir = args.be_dir
    block_size = args.block_size

    # get image size
    image_size = get_image_size()

    # establish rescaler for going from image offset to data index
    rescaler = 1.0 * IMAGE_SIZE / (image_size * block_size)

    # read and set the data
    set_data(data)

    # convert data to 2D array
    plottable_data = data.reshape(int(math.sqrt(IMAGE_SIZE)),-1)

    # make plot
    # http://matplotlib.org/examples/pylab_examples/colorbar_tick_labelling_demo.html
    fig, ax = matplotlib.pyplot.subplots()
    print(type(plottable_data))
    print(plottable_data.shape)
    print("data2 %s" % data[2])
    #cax = ax.imshow(plottable_data, cmap=matplotlib.cm.Greys)
    cax = ax.imshow(plottable_data, cmap=matplotlib.cm.Blues)
    ax.set_title('Matches in %s byte image' %image_size)

    # show
    matplotlib.pyplot.show()

