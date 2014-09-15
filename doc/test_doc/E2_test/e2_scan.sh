#!/bin/sh
# e2: scan mode testing
# Run on machine with plenty of RAM and cores
#
############################################################
# 1 million hashes
############################################################
# Create database
hashdb create scan_1m.hdb

# Add random hashes
hashdb add_random scan_1m.hdb 1000000

# Copy to second database
cp -r scan_1m.hdb scan_1m_copy.hdb

# Perform scan test
hashdb scan_random scan_1m.hdb scan_1m_copy.hdb

# Generate timing graph
../../../python/plot_log_timestamp.py scan_1m.hdb/log.xml scan_1m_graph

############################################################
# 10 million hashes
############################################################
# Create database
#hashdb create scan_10m.hdb

# Add random hashes
#hashdb add_random scan_10m.hdb 10000000

# Copy to second database
#cp -r scan_10m.hdb scan_10m_copy.hdb

# Perform scan test
#hashdb scan_random scan_10m.hdb scan_10m_copy.hdb

# Generate timing graph
#../../../plot_log_timestamp.py scan_10m_copy.hdb

