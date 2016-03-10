#!/usr/bin/env python
# Note: test set currently tests by inspection or failure.

import hashdb
import shutil
print("Begin")

# Support functions
# Support function: Version
print("Version: %s" % hashdb.version())

# Support function: Create new zz.hdb
hashdb_dir="zz.hdb"
shutil.rmtree(hashdb_dir, True)
cmd="my command string"
settings = hashdb.settings_t()
error_message = hashdb.create_hashdb(hashdb_dir, settings, cmd)
if len(error_message) == 0:
    print("Created .'%s'" % error_message)
else:
    print("Error: %s" % error_message)

# Support function: read settings
hashdb_dir="zz.hdb"
error_message = hashdb.read_settings(hashdb_dir, settings)
if len(error_message) == 0:
    print settings.max_source_offset_pairs
else:
    print("Error reading settings: %s" % error_message)

# Support function: test conversion to binary and back
binary_string = hashdb.hex_to_bin("0000000000000000")
print("binary string: '%s'" % binary_string)
hex_string = hashdb.bin_to_hex(binary_string)
print("hex to bin and back: '%s'" % hex_string)

# import
import_manager = hashdb.import_manager_t(hashdb_dir, "insert test data")
import_manager.insert_source_name("hhhhhhhh", "rn1", "fn1")
import_manager.insert_source_name("hhhhhhhh", "rn2", "fn2")
import_manager.insert_source_data("hhhhhhhh", 100, "ft1", 1)
import_manager.insert_hash("hhhhhhhh","gggggggg", 512, 2, "block label")
import_manager.insert_hash_json('{"block_hash":"6868686868686868","entropy":2,"block_label":"block label","source_offset_pairs":["6767676767676767",512]}')
import_manager.insert_source_json('"file_hash":"6767676767676767","filesize":0,"file_type":"","nonprobative_count":0,"name_pairs":[]')

# scan
scan_manager = hashdb.scan_manager_t(hashdb_dir)
expanded_hash = scan_manager.find_expanded_hash("hhhhhhhh")
print("expanded hash: %s" % expanded_hash)
json_hash_string = scan_manager.find_hash_json("hhhhhhhh")
print("json hash string: %s" % json_hash_string)
print("approximate hash count: %s" %
                     scan_manager.find_approximate_hash_count("hhhhhhhh"))
print("hash count: %s" % scan_manager.find_hash_count("hhhhhhhh"))
print("find_source_json: %s" % scan_manager.find_source_json("gggggggg"))

first_binary_hash = scan_manager.first_hash()
print("first_binary_hash %s" % first_binary_hash)
print("first_binary_hash %s" % first_binary_hash)
print("next binary hash '%s'" % scan_manager.next_hash(first_binary_hash))

first_file_binary_hash = scan_manager.first_hash()
print("first_file_binary_hash %s" % first_file_binary_hash)
print("first_file_binary_hash %s" % hashdb.bin_to_hex(first_file_binary_hash))
print("next file_binary hash '%s'" %
                     scan_manager.next_hash(first_file_binary_hash))

print("sizes: %s" % scan_manager.sizes())
print("hash size: %s" % scan_manager.size_hashes())
print("source size: %s" % scan_manager.size_sources())

# Settings
settings.sector_size = 1
settings.block_size = 2
settings.max_source_offset_pairs = 3
settings.hash_prefix_bits = 4
settings.hash_suffix_bytes = 5
print("Settings: %s" % settings.settings_string())

# Timestamp
ts = hashdb.timestamp_t()
print("timestamp1: %s" % ts.stamp("time1"))
print("timestamp2: %s" % ts.stamp("time2"))

