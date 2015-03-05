#!/usr/bin/python
#
# hashdb_helpers.py
# A module for helping with hashdb tests
import xml.etree.ElementTree as ET
from subprocess import Popen, PIPE
import os

# run command array and return lines from it
def hashdb(cmd):
    # find hashdb
    if os.path.isfile("../hashdb"):
        # path for "make check" from base dir
        cmd.insert(0, "../hashdb")
    elif os.path.isfile("../hashdb.exe"):
        # win path for "make check" from base dir
        cmd.insert(0, "../hashdb.exe")
    elif os.path.isfile("./hashdb"):
        # path for "make check" from test dir
        cmd.insert(0, "./hashdb")
    elif os.path.isfile("./hashdb.exe"):
        # win path for "make check" from test dir
        cmd.insert(0, "./hashdb.exe")
    else:
        print("hashdb tool not found.  Aborting.\n")
        exit(1)

    # run hashdb command
    p = Popen(cmd, stdout=PIPE)
    lines = p.communicate()[0].decode('utf-8').split("\n")
    if p.returncode != 0:
        print("error with command '", end="")
        print(*cmd, sep=' ', end="':\n")
        print(*lines, sep='\n')
        print("Aborting.")
        raise Exception("hashdb aborted.")

    return lines # note: last line is blank, hashdb does not generate it.

def parse_settings(hashdb_dir):
    settings = {}
    # test settings fields
    tree = ET.parse(hashdb_dir+"/settings.xml")
    root = tree.getroot()

    settings['settings_version'] = int(root.find('settings_version').text)
    settings['byte_alignment'] = int(root.find('byte_alignment').text)
    settings['hash_truncation'] = int(root.find('hash_truncation').text)
    settings['hash_block_size'] = int(root.find('hash_block_size').text)
    settings['maximum_hash_duplicates'] = int(root.find('maximum_hash_duplicates').text)
    temp = root.find('bloom_used').text
    if temp == "enabled":
        settings['bloom_used'] = True
    elif temp == "disabled":
        settings['bloom_used'] = False
    else:
        raise ValueError("invalid state '" + temp + "'")
    settings['bloom_k_hash_functions'] = int(root.find('bloom_k_hash_functions').text)
    settings['bloom_M_hash_size'] = int(root.find('bloom_M_hash_size').text)
    return settings

def parse_sizes(lines):
    sizes = {}
    for line in lines:
        if line.startswith("hash store size: "):
            sizes['hash_store_size'] = int(line[17:])
        if line.startswith("source store size: "):
            sizes['source_store_size'] = int(line[19:])
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

def dfxml_hash_equals(
                      repository_name="repositoryname",
                      filename="file1",
                      filesize=0,
                      file_hashdigest_type="MD5",
                      file_hashdigest="ff112233445566778899aabbccddeeff",
                      byte_run_file_offset=0,
                      byte_run_len=4096,
                      byte_run_hash_label="",
                      byte_run_hashdigest_type="MD5",
                      byte_run_hashdigest="00112233445566778899aabbccddeeff"):
    tree = ET.parse("temp_1.xml")
    root = tree.getroot()
    fileobject_node = root.find('fileobject')
    str_equals(fileobject_node.find('repository_name').text, repository_name)
    str_equals(fileobject_node.find('filename').text, filename)

    # filesize is optional, default 0
    if (fileobject_node.find('filesize') == None):
        # require default = 0
        int_equals(filesize, 0)
    else:
        # use filesize
        int_equals(int(fileobject_node.find('filesize').text), filesize)

    # file hashdigest is optional
    if (fileobject_node.find('hashdigest') == None):
        # require default = ""
        str_equals(file_hashdigest_type, "");
        str_equals(file_hashdigest, "");
    else:
        str_equals(fileobject_node.find('hashdigest').attrib['type'], file_hashdigest_type)
        str_equals(fileobject_node.find('hashdigest').text, file_hashdigest)

    byte_run_node = fileobject_node.find('byte_run')
    str_equals(int(byte_run_node.attrib['file_offset']), byte_run_file_offset)
    str_equals(int(byte_run_node.attrib['len']), byte_run_len)
    # presence of hash label attribute is optional
    if 'label' in byte_run_node.attrib:
        # check that labels match
        str_equals(byte_run_node.attrib['label'], byte_run_hash_label)
    else:
        # require default = ""
        str_equals(byte_run_hash_label, "");
    str_equals(byte_run_node.find('hashdigest').attrib['type'], byte_run_hashdigest_type)
    str_equals(byte_run_node.find('hashdigest').text, byte_run_hashdigest)

# write one block hash entry to file temp_dfxml_hash
def write_temp_dfxml_hash(
                          repository_name="repositoryname",
                          filename="file1",
                          filesize=0,
                          file_hashdigest_type="MD5",
                          file_hashdigest="ff112233445566778899aabbccddeeff",
                          byte_run_file_offset=0,
                          byte_run_len=4096,
                          byte_run_hash_label="",
                          byte_run_hashdigest_type="MD5",
                          byte_run_hashdigest="00112233445566778899aabbccddeeff"):
    tempfile = open("temp_dfxml_hash", "w")
    tempfile.write("<?xml version='1.0' encoding='UTF-8'?>\n")
    tempfile.write("<dfxml xmloutputversion='1.0'>\n")
    tempfile.write("  <fileobject>\n")
    tempfile.write("    <filename>" + filename + "</filename>\n")
    tempfile.write("    <repository_name>" + repository_name +
                                "</repository_name>\n")
    tempfile.write("    <filesize>" + str(filesize) + "</filesize>\n")
    tempfile.write("    <hashdigest type='" + file_hashdigest_type + "'>" +
                                file_hashdigest + "</hashdigest>\n")
    tempfile.write("    <byte_run file_offset='" + str(byte_run_file_offset) +
                                "' len='" + str(byte_run_len) + "'")
    if byte_run_hash_label != "":
        tempfile.write(" label='" + byte_run_hash_label + "'")
    tempfile.write(">\n")
    tempfile.write("      <hashdigest type='" + byte_run_hashdigest_type +
                                "'>" + byte_run_hashdigest + "</hashdigest>\n")
    tempfile.write("    </byte_run>\n")
    tempfile.write("  </fileobject>\n")
    tempfile.write("</dfxml>\n")

# write one block hash entry to file temp_tab_hash
def write_temp_tab_hash(
                          filename="file1",
                          block=1,
                          hashdigest="00112233445566778899aabbccddeeff"):
    tempfile = open("temp_tab_hash", "w")
    tempfile.write(filename + "\t" + hashdigest + "\t" + str(block))

def rm_tempfile(filename):
    if filename[:5] != "temp_":
        # safeguard
        print("aborting, invalid filename:", filename)
        exit(1)
    if os.path.exists(filename):
        os.remove(filename)
 
