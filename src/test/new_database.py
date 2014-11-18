#!/usr/bin/python
#
# Test the "New Database" command group

#from subprocess import call
import subprocess
import xml.etree.ElementTree as ET

# check that input parameters get into settings.xml
def test_create():
    db1 = "temp1.hdb"

    # create new db
    subprocess.call(["rm", "-rf", db1])
    subprocess.check_call(["hashdb", "create", db1, "-p512", "-m10", "--bloom=disabled", "--bloom_kM=4:14"])

    # test settings fields
    tree = ET.parse(db1+"/settings.xml")
    root = tree.getroot()

    byte_alignment = int(root.find('byte_alignment').text)
#    print "byte_alignment:",byte_alignment
    if byte_alignment != 512:
        raise ValueError("invalid byte_alignment: " + str(byte_alignment))

    hash_block_size = int(root.find('hash_block_size').text)
    if hash_block_size != 512:
        raise ValueError("invalid hash_block_size: " + str(hash_block_size))

    maximum_hash_duplicates = int(root.find('maximum_hash_duplicates').text)
    if maximum_hash_duplicates != 10:
        raise ValueError("invalid maximum_hash_duplicates: " + str(maximum_hash_duplicates))

    bloom1_used = root.find('bloom1_used').text
    if bloom1_used != 'disabled':
        raise ValueError("invalid bloom1_used: " + str(bloom1_used))

    bloom1_k_hash_functions = int(root.find('bloom1_k_hash_functions').text)
    if bloom1_k_hash_functions != 4:
        raise ValueError("invalid bloom1_k_hash_functions: " + str(bloom1_k_hash_functions))

    bloom1_M_hash_size = int(root.find('bloom1_M_hash_size').text)
    if bloom1_M_hash_size != 14:
        raise ValueError("invalid bloom1_M_hash_size: " + str(bloom1_M_hash_size))

    # cleanup
    subprocess.call(["rm", "-rf", db1])
    print "Test Done."

if __name__=="__main__":
    test_create()

