#!/usr/bin/env python

# To run this test in place but outside the "make check" test harness:
#   1) type: export PYTHONPATH='.:.libs'
#   2) type: ../../python_bindings/test_interfaces.py

import hashdb
import shutil
from struct import pack

# require equality, adapted from ../test_py/helpers.py
def str_equals(a,b):
    if a != b:
        raise ValueError("'%s' not equal to '%s'" % (a,b))
def bool_equals(a,b):
    if a != b:
        raise ValueError(a + " not equal to " + b)
def int_equals(a,b):
    if a != b:
        raise ValueError(str(a) + " not equal to " + str(b))


# Support functions
# Support function: Version
str_equals(hashdb.version()[:2], "3.")

# Support function: Create new temp_1.hdb
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

# import
import_manager = hashdb.import_manager_t("temp_1.hdb", "insert test data")
import_manager.insert_source_name("hhhhhhhh", "rn1", "fn1")
import_manager.insert_source_name("hhhhhhhh", "rn2", "fn2")
import_manager.insert_source_data("hhhhhhhh", 100, "ft1", 1)
import_manager.insert_hash("hhhhhhhh","gggggggg", 512, 2, "block label")
import_manager.import_json('{"block_hash":"6868686868686868","entropy":2,"block_label":"block label","source_offset_pairs":["6767676767676767",512]}')
import_manager.import_json('"file_hash":"6767676767676767","filesize":0,"file_type":"","nonprobative_count":0,"name_pairs":[]')
str_equals(import_manager.size(), '{"hash_data_store":1, "hash_store":1, "source_data_store":2, "source_id_store":2, "source_name_store":2}')

# scan
scan_manager = hashdb.scan_manager_t("temp_1.hdb")
expanded_hash = scan_manager.find_expanded_hash_json("hhhhhhhh")
str_equals(expanded_hash,
'{"block_hash":"6868686868686868","entropy":2,"block_label":"block label","source_list_id":3724381083,"sources":[{"file_hash":"6767676767676767","filesize":0,"file_type":"","nonprobative_count":0,"name_pairs":[]}],"source_offset_pairs":["6767676767676767",512]}'
)

json_hash_string = scan_manager.export_hash_json("hhhhhhhh")
str_equals(json_hash_string, '{"block_hash":"6868686868686868","entropy":2,"block_label":"block label","source_offset_pairs":["6767676767676767",512]}')

int_equals(scan_manager.find_approximate_hash_count("hhhhhhhh"), 1)

int_equals(scan_manager.find_hash_count("hhhhhhhh"), 1)

str_equals(scan_manager.export_source_json("gggggggg"), '{"file_hash":"6767676767676767","filesize":0,"file_type":"","nonprobative_count":0,"name_pairs":[]}')

first_binary_hash = scan_manager.first_hash()
str_equals(hashdb.bin_to_hex(first_binary_hash), "6868686868686868")

next_binary_hash = scan_manager.next_hash(first_binary_hash)
str_equals(hashdb.bin_to_hex(next_binary_hash), "")

## next after end is invalid and currently calls assert which is extreme.
#next_binary_hash = scan_manager.next_hash(next_binary_hash)

first_binary_source = scan_manager.first_source()
str_equals(hashdb.bin_to_hex(first_binary_source), "6767676767676767")

next_binary_source = scan_manager.next_source(first_binary_source)
str_equals(hashdb.bin_to_hex(next_binary_source), "6868686868686868")

next_binary_source = scan_manager.next_source(next_binary_source)
str_equals(hashdb.bin_to_hex(next_binary_source), "")

str_equals(scan_manager.size(), '{"hash_data_store":1, "hash_store":1, "source_data_store":2, "source_id_store":2, "source_name_store":2}')
int_equals(scan_manager.size_hashes(), 1)
int_equals(scan_manager.size_sources(), 2)

# scan_fd, end with EOF
temp_fd = open("temp_in.bin", "w+b")
in_bytes = pack('8sQ', 'aaaaaaaa', 1)
temp_fd.write(in_bytes)
in_bytes = pack('8sQ', 'hhhhhhhh', 1)
temp_fd.write(in_bytes)
temp_fd.flush()
in_fd = open("temp_in.bin", "r+b")
out_fd = open("temp_out.json", "w")
# zz scan_manager.scan_fd(in_fd, out_fd, 8, 8, hashdb.EXPANDED_HASH)

# scan_fd, end with 0x00... zz


# Settings
settings.byte_alignment = 1
settings.block_size = 2
settings.max_source_offset_pairs = 3
settings.hash_prefix_bits = 4
settings.hash_suffix_bytes = 5
str_equals(settings.settings_string(), '{"settings_version":3, "byte_alignment":1, "block_size":2, "max_source_offset_pairs":3, "hash_prefix_bits":4, "hash_suffix_bytes":5}')

# Timestamp
ts = hashdb.timestamp_t()
str_equals(ts.stamp("time1")[:15], '{"name":"time1"')
str_equals(ts.stamp("time2")[:15], '{"name":"time2"')

