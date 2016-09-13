#!/usr/bin/env python

# This script tests the Python version and hashdb library functions.
# This script creates filenames starting with "temp_" in the local directory.

# ############################################################
# test Python version
# ############################################################
import sys
# require Python version 2.7
if sys.version_info.major != 2 and sys.version_info.minor != 7:
    print("Error: Invalid Python version: 2.7 is required.")
    print(sys.version_info)
    sys.exit(1)

# require 64-bit Python
if sys.maxsize != 2**63 - 1:
    print("Error: 64-bit Python is required.")
    print("found %d but expected %d" % (sys.maxsize, 2**64))
    sys.exit(1)

import hashdb
import shutil
import struct
import io
import os

# require equality, adapted from ../test_py/helpers.py
def str_equals(a,b):
    if a != b:
        print("a", ":".join("{:02x}".format(ord(c)) for c in a))
        print("b", ":".join("{:02x}".format(ord(c)) for c in b))
        raise ValueError("'%s' not equal to '%s'" % (a,b))
def bool_equals(a,b):
    if a != b:
        raise ValueError(a + " not equal to " + b)
def int_equals(a,b):
    if a != b:
        raise ValueError(str(a) + " not equal to " + str(b))

# ############################################################
# test Support functions
# ############################################################

# Support function: Version
str_equals(hashdb.version()[:2], "3.")
str_equals(hashdb.hashdb_version()[:2], "3.")

# Support function: Create new hashdb temp_1.hdb
shutil.rmtree("temp_1.hdb", True)
cmd="my command string"
settings = hashdb.settings_t()
str_equals(hashdb.create_hashdb("temp_1.hdb", settings, cmd), "")

# Support function: read settings
str_equals(hashdb.read_settings("temp_1.hdb", settings), "")

# Support function: test conversion to binary and back
binary_string = hashdb.hex_to_bin("0000000000000000")
hex_string = hashdb.bin_to_hex(binary_string)
str_equals(hex_string, "0000000000000000")

# support function: ingest, scan_media: not tested

# support function: read_media
# setup for read_media, end with EOF
temp_in = open("temp_in.bin", "w")
in_bytes = struct.pack('5s', '\0\0a\0\0')
temp_in.write(in_bytes)
temp_in.close()

# read_media using numeric offset
error_message, bytes_read = hashdb.read_media("temp_in.bin", 0, 10)
str_equals(error_message,"")
str_equals(bytes_read, b'\0\0a\0\0')
error_message, bytes_read = hashdb.read_media("temp_in.bin", 2, 2)
str_equals(error_message,"")
str_equals(bytes_read, b'a\0')
error_message, bytes_read = hashdb.read_media("temp_in.bin", 10, 10)
str_equals(error_message,"")
str_equals(bytes_read, b'')
error_message, bytes_read = hashdb.read_media("temp_invalid_fileanme", 10, 10)
bool_equals(len(error_message) > 0, True)
str_equals(bytes_read, b'')

# read_media using media offset
error_message, bytes_read = hashdb.read_media("temp_in.bin", "0", 10)
str_equals(error_message,"")
str_equals(bytes_read, b'\0\0a\0\0')
error_message, bytes_read = hashdb.read_media("temp_in.bin", "0-zip-0", 10)
str_equals(error_message,"zip region too small")
str_equals(bytes_read, "")

# ############################################################
# test import functions
# ############################################################
import_manager = hashdb.import_manager_t("temp_1.hdb", "insert test data")
import_manager.insert_source_name("tttttttt", "rn1", "fn1")
import_manager.insert_source_name("tttttttt", "rn2", "fn2")
import_manager.insert_source_data("tttttttt", 100, "ft1", 11, 1)
import_manager.insert_hash("vvvvvvvv", 8000, "a_blocklabel","wwwwwwww", 512)
import_manager.import_json('{"block_hash":"6868686868686868","k_entropy":2000,"block_label":"blocklabel","source_offsets":["7373737373737373",1,[512]]}')
import_manager.import_json('"file_hash":"7373737373737373","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]')
bool_equals(import_manager.has_source("tttttttt"), True)
first_binary_source = import_manager.first_source()
str_equals(hashdb.bin_to_hex(first_binary_source), "7373737373737373")
next_binary_source = import_manager.next_source(first_binary_source)
str_equals(hashdb.bin_to_hex(next_binary_source), "7474747474747474")
str_equals(import_manager.size(), '{"hash_data_store":2, "hash_store":2, "source_data_store":3, "source_id_store":3, "source_name_store":2}')
str_equals(import_manager.size_hashes(), 2)
str_equals(import_manager.size_sources(), 3)
import_manager = None

# ############################################################
# test scan functions
# ############################################################
scan_manager = hashdb.scan_manager_t("temp_1.hdb")
expanded_hash = scan_manager.find_hash_json(hashdb.EXPANDED_OPTIMIZED, "hhhhhhhh")
str_equals(expanded_hash,
'{"block_hash":"6868686868686868","k_entropy":2000,"block_label":"blocklabel","count":1,"source_list_id":1268430100,"sources":[{"file_hash":"7373737373737373","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_offsets":["7373737373737373",1,[512]]}'
)

json_hash_string = scan_manager.export_hash_json("hhhhhhhh")
str_equals(json_hash_string, '{"block_hash":"6868686868686868","k_entropy":2000,"block_label":"blocklabel","source_offsets":["7373737373737373",1,[512]]}')

int_equals(scan_manager.find_hash_count("hhhhhhhh"), 1)

str_equals(scan_manager.find_hash_json(hashdb.COUNT, "hhhhhhhh"), '{"block_hash":"6868686868686868","count":1}')

int_equals(scan_manager.find_approximate_hash_count("hhhhhhhh"), 1)

str_equals(scan_manager.find_hash_json(hashdb.APPROXIMATE_COUNT, "hhhhhhhh"), '{"block_hash":"6868686868686868","approximate_count":1}')

has_source_data, filesize, file_type, zero_count, nonprobative_count = scan_manager.find_source_data("tttttttt")
bool_equals(has_source_data, True)
int_equals(filesize, 100)
str_equals(file_type, "ft1")
int_equals(zero_count, 11)
int_equals(nonprobative_count, 1)

str_equals(scan_manager.export_source_json("ssssssss"), '{"file_hash":"7373737373737373","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}')

first_binary_hash = scan_manager.first_hash()
str_equals(hashdb.bin_to_hex(first_binary_hash), "6868686868686868")

next_binary_hash = scan_manager.next_hash(first_binary_hash)
next_binary_hash = scan_manager.next_hash(next_binary_hash)
str_equals(hashdb.bin_to_hex(next_binary_hash), "")

first_binary_source = scan_manager.first_source()
str_equals(hashdb.bin_to_hex(first_binary_source), "7373737373737373")

next_binary_source = scan_manager.next_source(first_binary_source)
str_equals(hashdb.bin_to_hex(next_binary_source), "7474747474747474")

next_binary_source = scan_manager.next_source(next_binary_source)
next_binary_source = scan_manager.next_source(next_binary_source)
str_equals(hashdb.bin_to_hex(next_binary_source), "")

str_equals(scan_manager.size(), '{"hash_data_store":2, "hash_store":2, "source_data_store":3, "source_id_store":3, "source_name_store":2}')
int_equals(scan_manager.size_hashes(), 2)
int_equals(scan_manager.size_sources(), 3)

def temp_out_equals(a):
    infile = open("temp_out", "r")
    str_equals(infile.read().strip(), a)
    infile.close()


# Settings
settings.byte_alignment = 1
settings.block_size = 2
settings.max_count = 3
settings.max_sub_count = 4
settings.hash_prefix_bits = 5
settings.hash_suffix_bytes = 6
str_equals(settings.settings_string(), '{"settings_version":3, "byte_alignment":1, "block_size":2, "max_count":3, "max_sub_count":4, "hash_prefix_bits":5, "hash_suffix_bytes":6}')

# Timestamp
ts = hashdb.timestamp_t()
str_equals(ts.stamp("time1")[:15], '{"name":"time1"')
str_equals(ts.stamp("time2")[:15], '{"name":"time2"')

# ############################################################
# test scan_stream
# ############################################################
# setup for scan_stream
in_bytes_a = struct.pack('8sH10s', 'aaaaaaaa', 10, 'iiiiiiiiii') # not present
in_bytes_h = struct.pack('8sH10s', 'hhhhhhhh', 10, 'iiiiiiiiii') # present

# helper to read one scanned line from scan_stream
def read_scan_stream(scan_stream):
    while True:
        scanned = scan_stream.get()
        if scanned != "":
            return scanned
        if scan_stream.empty():
            raise AssertionError("no data")

# scan_stream EXPANDED
scan_stream = hashdb.scan_stream_t(scan_manager, 8, hashdb.EXPANDED)
scan_stream.put(in_bytes_a)  # check that the unfound value does not get in the way
scan_stream.put(in_bytes_h)
scan_stream.put(in_bytes_h)
scanned = read_scan_stream(scan_stream)
str_equals(scanned, 'hhhhhhhh\n\x00iiiiiiiiii\x20\x01\x00\x00{"block_hash":"6868686868686868","k_entropy":2000,"block_label":"blocklabel","count":1,"source_list_id":1268430100,"sources":[{"file_hash":"7373737373737373","filesize":0,"file_type":"","zero_count":0,"nonprobative_count":0,"name_pairs":[]}],"source_offsets":["7373737373737373",1,[512]]}')
int_equals(len(scanned), 312)
scanned = read_scan_stream(scan_stream)
int_equals(len(scanned), 312)
bool_equals(scan_stream.empty(), True)
scan_stream.put(in_bytes_h)  # check second put
scanned = read_scan_stream(scan_stream)
int_equals(len(scanned), 312)

# can check by hand: add data to verify warning that stream is not empty
#scan_stream.put(in_bytes_h)
#exit(1)

# scan_stream EXPANDED_OPTIMIZED
scan_manager = hashdb.scan_manager_t("temp_1.hdb") # reset EXPANDED_OPTIMIZED
scan_stream = hashdb.scan_stream_t(scan_manager, 8, hashdb.EXPANDED_OPTIMIZED)
scan_stream.put(in_bytes_h)
scan_stream.put(in_bytes_h)
scanned1 = read_scan_stream(scan_stream) # 57 can come up first
scanned2 = read_scan_stream(scan_stream) # 312 can come up second
int_equals(len(scanned1) + len(scanned2), 312 + 57)

# scan_stream COUNT
scan_stream = hashdb.scan_stream_t(scan_manager, 8, hashdb.COUNT)
scan_stream.put(in_bytes_h)
scan_stream.put(in_bytes_h)
scanned = read_scan_stream(scan_stream)
int_equals(len(scanned), 67)
scanned = read_scan_stream(scan_stream)
int_equals(len(scanned), 67)

# scan_stream APPROXIMATE_COUNT
scan_stream = hashdb.scan_stream_t(scan_manager, 8, hashdb.APPROXIMATE_COUNT)
scan_stream.put(in_bytes_h)
scan_stream.put(in_bytes_h)
scanned = read_scan_stream(scan_stream)
int_equals(len(scanned), 79)
scanned = read_scan_stream(scan_stream)
int_equals(len(scanned), 79)

print("Done.")

