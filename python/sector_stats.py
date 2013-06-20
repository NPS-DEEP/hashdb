#!/usr/bin/env python3.2

'''
Program that takes a directory and block size and generates statistics about distinct blocks.

Arguments:
 --dir : Directory of files that will be analyzed (files should have file extensions in the format ".xyz".
 --bs : Specifies a block size for piecewise hashing

Output:
  - singletons.txt: A list of  singleton (only appear once in the files) file names and offsets
  - pairs.txt: A list of pairs (appear twice in the files) file names and offsets.
  - triples.txt: A list of triples (appear three times in all files) file names and offsets.
  - others.txt: A list of sectors that appeared more than three times in all files, with file names and offsets. 
'''

class SectorCorrelator:
    SINGLETONINDEX = 0
    PAIRINDEX = 1
    TRIPLEINDEX = 2
    OTHERINDEX = 3
    TMP_FILETYPE = "tmp_filetype.txt"
    TMP_FILENAME = "tmp_filename.txt"

    def __init__(self):
        import collections
        self.hashdb = collections.defaultdict(list) #  key is the MD5 code, value is a list of matches
        self.filedb = collections.defaultdict(list)
        self.filetypedb = collections.defaultdict(list)
        self.files = 0
        self.sectors = 0
        
    def process(self,fi):
        """ Process the <fileobject> objects as they are read from the XML file"""
        self.files += 1
        if self.files % 100 == 0:
            print("Done processing {} files".format(self.files))
        # print(fi.filename())
        for br in fi.byte_runs():
            self.sectors += 1
            # ignore null blocks
            if br.hashdigest['md5'] != 'bf619eac0cdf3f68d496ea9344137e8b':
                self.hashdb[br.hashdigest['md5']].append((fi.filename(),br.file_offset))

    def gen_file_stats(self):
        f_tmp = open(self.TMP_FILENAME, "rt")
        for line in f_tmp:
            filenames = line.split()
            for file in filenames:
                count = filenames.count(file)
                if file not in self.filedb:
                    self.filedb[file]=[0,0,0,0]
                if count == 1:
                    self.filedb[file][self.SINGLETONINDEX] += 1
                elif count == 2:
                    self.filedb[file][self.PAIRINDEX] += 1
                elif count == 3:
                    self.filedb[file][self.TRIPLEINDEX] += 1
                else:
                    self.filedb[file][self.OTHERINDEX] += 1
        f_tmp.close()

    def gen_filetype_stats(self):
        f_tmp = open(self.TMP_FILETYPE,"rt")
        for line in f_tmp:
            filetypes = line.split()
            count = len(filetypes)
            for type in filetypes:
                if type not in self.filetypedb:
                    self.filetypedb[type]=[0,0,0,0]
                if count == 1:
                    self.filetypedb[type][self.SINGLETONINDEX] += 1
                elif count == 2:
                    self.filetypedb[type][self.PAIRINDEX] += 1
                elif count == 3:
                    self.filetypedb[type][self.TRIPLEINDEX] += 1
                else:
                    self.filetypedb[type][self.OTHERINDEX] += 1
        f_tmp.close()


    def print_file_report(self):
        print("\nFile Statistics")
        print("Filename\t% singletons\t% pairs\t% triples\t% other")
        for (file,counts) in self.filedb.items():
            total = sum(counts)
            print("{}\t{}\t{}\t{}\t{}".format(file, 
                                             counts[self.SINGLETONINDEX]/total,
                                             counts[self.PAIRINDEX]/total,
                                             counts[self.TRIPLEINDEX]/total,
                                             counts[self.OTHERINDEX]/total))
        self.filedb = {} # to clear memory

    def print_filetype_report(self):
        print("\nFiletype Statistics")
        print("Filetype\t% singletons\t% pairs\t% triples\t% other")
        for (filetype,counts) in self.filetypedb.items():
            total = sum(counts)
            print("{}\t{}\t{}\t{}\t{}".format(filetype, 
                                             counts[self.SINGLETONINDEX]/total,
                                             counts[self.PAIRINDEX]/total,
                                             counts[self.TRIPLEINDEX]/total,
                                             counts[self.OTHERINDEX]/total))
        self.filetypedb = {} # to clear memory
                                             
                    
    def print_report(self):
        self.singletons=0
        self.pairs=0
        self.triples=0
        self.others=0

        print("Files processed: {}".format(self.files))
        print("Sectors processed: {}".format(self.sectors))
        print("")
        print("The following duplicates were found:")
        print("Hash   Filename           Offset in file")

        f_singleton = open('singletons.txt', 'wt')
        f_pair = open('pairs.txt', 'wt')
        f_triple = open('triples.txt', 'wt')
        f_other = open('others.txt', 'wt')
        f_tmp_filetype = open(self.TMP_FILETYPE, 'wt')
        f_tmp_filename = open(self.TMP_FILENAME, 'wt')

        for (hash,ents) in self.hashdb.items():

            # count corpus-wide sector stats
            if len(ents) == 1:
                self.singletons += 1
                f_singleton.write("{}\t{}\n".format(ents[0][0],ents[0][1]))
            elif len(ents) == 2:
                self.pairs += 1
                f_pair.write("{}\t{}\n".format(ents[0][0],ents[0][1]))
            elif len(ents) == 3:
                self.triples += 1
                f_triple.write("{}\t{}\n".format(ents[0][0],ents[0][1]))
            else:
                self.others += 1
                f_other.write("{}\t{}\n".format(ents[0][0],ents[0][1]))
                
            if len(ents)>1:
                print("{}  -- {} copies found".format(hash,len(ents)))
                for e in sorted(ents):
                    print("  {}  {:8,}".format(e[0],e[1]))
                print("")


            filetypes_list = [x[0][x[0].rfind("."):][1:] for x in ents]
            for x in filetypes_list:
                f_tmp_filetype.write("{} ".format(x))
            f_tmp_filetype.write("\n")

            filenames_list = [x[0] for x in ents]
            for x in filenames_list:
                f_tmp_filename.write("{} ".format(x))
            f_tmp_filename.write("\n")
        
        f_singleton.close()
        f_pair.close()
        f_triple.close()
        f_other.close()
        f_tmp_filetype.close()
        f_tmp_filename.close()

        self.hasdb = {} # to clear the memory
        print ("singletons {}, pairs {}, triples {}, others{}".format(self.singletons, self.pairs, self.triples, self.others))


if __name__=="__main__":
    import argparse,os,sys
    sys.path.append(os.getenv("DOMEX_HOME") + "/src/lib/") # add the library
    sys.path.append(os.getenv("DOMEX_HOME") + "/src/dfxml/python/")                             # add the library
    import dfxml,subprocess

    parser = argparse.ArgumentParser(description="A program that takes a directory of files, computes sector-based statistics.")
    parser.add_argument("--dir", help="Directory of files that will be analyzed")
    parser.add_argument("--bs", help="Specifies a block size for piecewise hashing", type=int, default=512)
    parser.add_argument("--file", help="Specifies a file that contains output from md5deep", type=str, default=None)
    args = parser.parse_args()

    sc = SectorCorrelator()
    if args.file is None:
        p = subprocess.Popen(['md5deep','-dp '+ str(args.bs), '-r', args.dir],stdout=subprocess.PIPE)
        dfxml.read_dfxml(xmlfile=p.stdout,callback=sc.process)
    else:
        dfxml.read_dfxml(xmlfile=open(args.file,'rb'),callback=sc.process)

    sc.print_report()
    sc.gen_file_stats()
    sc.print_file_report()
    sc.gen_filetype_stats()
    sc.print_filetype_report()
