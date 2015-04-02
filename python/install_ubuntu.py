#!/usr/bin/python
# 
#  Ubuntu 14.04.02
#

import os

os.system('sudo apt-get -y install gcc g++ openssl flex libewf-dev libxml2-dev libssl-dev libtre-dev libtool p7zip')
os.system('wget http://digitalcorpora.org/downloads/hashdb/bulk_extractor-1.5.5-dev.tar.gz')
os.system('tar -zxvf bulk_extractor-1.5.5-dev.tar.gz')
os.system('cd bulk_extractor-1.5.5-dev && wget http://digitalcorpora.org/downloads/hashdb/hashdb-2.0.1.tar.gz && tar -zxvf hashdb-2.0.1.tar.gz')
os.system('cd bulk_extractor-1.5.5-dev/hashdb-2.0.1 && ./configure && make && sudo make install')
os.system('cd bulk_extractor-1.5.5-dev && ./configure && make && sudo make install')
