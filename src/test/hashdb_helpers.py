#!/usr/bin/python
#
# hashdb_helpers.py
# A module for helping with hashdb tests
import xml.etree.ElementTree as ET
import subprocess
from subprocess import Popen, PIPE

# run command array and return lines from it
def hashdb(cmd):
    cmd.insert(0, "hashdb")
    return Popen(cmd, stdout=PIPE).communicate()[0].decode('utf-8').split("\n")

def create(hashdb_dir):
    subprocess.call(["hashdb", "create", hashdb_dir])

def parse_settings(hashdb_dir):
    settings = {}
    # test settings fields
    tree = ET.parse(hashdb_dir+"/settings.xml")
    root = tree.getroot()

    settings['settings_version'] = int(root.find('settings_version').text)
    settings['hash_digest_type'] = root.find('hash_digest_type').text
    settings['byte_alignment'] = int(root.find('byte_alignment').text)
    settings['hash_block_size'] = int(root.find('hash_block_size').text)
    settings['maximum_hash_duplicates'] = int(root.find('maximum_hash_duplicates').text)
    temp = root.find('bloom1_used').text
    if temp == "enabled":
        settings['bloom1_used'] = True
    elif temp == "disabled":
        settings['bloom1_used'] = False
    else:
        raise ValueError("invalid state '" + temp + "'")
    settings['bloom1_k_hash_functions'] = int(root.find('bloom1_k_hash_functions').text)
    settings['bloom1_M_hash_size'] = int(root.find('bloom1_M_hash_size').text)
    return settings

# require a specific database size
def parse_size(lines):
    sizes = {}
    for line in lines:
        line = stream.readline()
        if line.startswith("hash store: "):
            sizes['hash_store'] = int(line[12:])
        if line.startswith("source lookup store: "):
            sizes['source_lookup_store'] = int(line[21:])
        if line.startswith("source repository name store: "):
            sizes['source_repository_name_store'] = int(line[30:])
        if line.startswith("source filename store: "):
            sizes['source_filename_store'] = int(line[23:])
        if line.startswith("source metadata store: "):
            sizes['source_metadata_store'] = int(line[23:])
    return sizes

# test import
def parse_changes(lines):

    # create new db
    changes = {}
    for line in lines:
        if line.startswith("    hashes inserted: "):
            changes['hashes_inserted'] = int(line[21:])
        if line.startswith("    hashes not inserted (mismatched hash block size): "):
            changes['hashes_not_inserted_mismatched_hash_block_size'] = int(line[54:])
        if line.startswith("    hashes not inserted (invalid byte alignment): "):
            changes['hashes_not_inserted_invalid_byte_alignment'] = int(line[50:])
        if line.startswith("    hashes not inserted (exceeds max duplicates): "):
            changes['hashes_not_inserted_exceeds_max_duplicates'] = int(line[50:])
        if line.startswith("    hashes not inserted (duplicate element): "):
            changes['hashes_not_inserted_duplicate_element'] = int(line[45:])
        if line.startswith("    hashes removed: "):
            changes['hashes_removed'] = int(line[20:])
        if line.startswith("    hashes not removed (mismatched hash block size): "):
            changes['hashes_not_removed_mismatched_hash_block_size'] = int(line[53:])
        if line.startswith("    hashes not removed (invalid byte alignment): "):
            changes['hashes_not_removed_invalid_byte_alignment'] = int(line[49:])
        if line.startswith("    hashes not removed (no hash): "):
            changes['hashes_not_removed_no_hash'] = int(line[34:])
        if line.startswith("    hashes not removed (no element): "):
            changes['hashes_not_removed_no_element'] = int(line[37:])
        if line.startswith("    source metadata inserted: "):
            changes['source_metadata_inserted'] = int(line[30:])
        if line.startswith("    source metadata not inserted (already present): "):
            changes['source_metadata_not_inserted_already_present'] = int(line[52:])
    return changes

# require equality
def str_equals(a,b):
    if a != b:
        raise ValueError(a + " not equal to " + b)
def bool_equals(a,b):
    if a != b:
        raise ValueError(bool(a) + " not equal to " + bool(b))
def int_equals(a,b):
    if a != b:
        raise ValueError(str(a) + " not equal to " + str(b))


