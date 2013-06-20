#!/usr/bin/env python3.2

import sys, math, collections

'''
Program that takes a file with filenames and offsets and a block size and calculates entropy of bytes for each block at the offsets indicated in the input file.

Arguments:
 --file : Specifies a file that lists file names and offsets
 --bs : Specifies a block size for the offsets

Output: 
 -- the entropy distribution of the blocks specified by the provided file names and offsets 
'''
class ByteEntropy:
    def __init__(self,file,blocksize):
        self.file = file
        self.count = 0
        self.bs = blocksize
        self.distdb = collections.defaultdict(int)

    def process(self):
        f = open(self.file,'r')
        count=0
        for line in f.readlines():
            count += 1
            offset = int(line.split()[1])
            file = line.split()[0]
            entropy = self.compute_entropy(file,offset)
            self.distdb[entropy] += 1
            #print("{} {} {}".format(file, offset, entropy))
        f.close()
                 
    def print_entropy_distrib(self):
        key_list=list(self.distdb.keys())
        key_list.sort()
        for entropy_val in key_list:
            print("{}\t{}".format(entropy_val, 
                                  self.distdb[entropy_val]))

    def compute_entropy(self, file, offset):
        f = open(file, 'rb')
        f.seek(offset)
        myset = set()

        for count in range(self.bs):
            byte=f.read(1)
            if not byte:
                break
            myset.add(byte)

        f.close()

        return len(myset)/256

    
if __name__=="__main__":
    import argparse

    parser = argparse.ArgumentParser(description="A Byte-Entropy Calulation Program")
    parser.add_argument("--file",help="Specifies a file that lists file names and offsets", type=str)
    parser.add_argument("--bs", help="Specifies a block size for the offsets", type=int, default=512)
    args = parser.parse_args()

    be = ByteEntropy(args.file, args.bs)
    be.process()
    be.print_entropy_distrib()

