#!/bin/sh
# Validate this test by visually inspecting output file temp_out.
# Tests should indicate success, for example:
# ==25086== All heap blocks were freed -- no leaks are possible
# ==25086== ERROR SUMMARY: 0 errors from 0 contexts

RUN="valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=vg.supp hashdb"

# clear previous run
rm -rf temp_vg*

# New Database
$RUN create temp_vg.hdb 2>> temp_vg.out

# Import/Export
echo "1122334455667788	99aabbccddeeff	1" > temp_vg.tab
$RUN import_tab temp_vg.hdb temp_vg.tab 2>> temp_vg.out
$RUN ingest temp_vg.hdb temp_vg.tab 2>> temp_vg.out
$RUN export temp_vg.hdb temp_vg.json 2>> temp_vg.out
$RUN import temp_vg.hdb temp_vg.json 2>> temp_vg.out

# Database Manipulation
$RUN add temp_vg.hdb temp_vg2.hdb 2>> temp_vg.out
$RUN add_multiple temp_vg.hdb temp_vg2.hdb temp_vg3.hdb 2>> temp_vg.out
$RUN add_repository temp_vg.hdb temp_vg2.hdb "repository1" 2>> temp_vg.out
$RUN add_range temp_vg.hdb temp_vg2.hdb 0:9 "repository1" 2>> temp_vg.out
$RUN intersect temp_vg.hdb temp_vg2.hdb temp_vg3.hdb 2>> temp_vg.out
$RUN intersect_hash temp_vg.hdb temp_vg2.hdb temp_vg3.hdb 2>> temp_vg.out
$RUN subtract temp_vg.hdb temp_vg2.hdb temp_vg3.hdb 2>> temp_vg.out
$RUN subtract_hash temp_vg.hdb temp_vg2.hdb temp_vg3.hdb 2>> temp_vg.out
$RUN subtract_repository temp_vg.hdb temp_vg2.hdb "repository1" 2>> temp_vg.out

# Scan
echo "media_offset_1	99aabbccddeeff" > temp_vg_hashes.txt
$RUN scan_list temp_vg.hdb temp_vg_hashes.txt 2>> temp_vg.out
$RUN scan_hash temp_vg.hdb "99aabbccddeeff" 2>> temp_vg.out
$RUN scan_media temp_vg.hdb temp_vg_hashes.txt 2>> temp_vg.out

# Statistics
$RUN size temp_vg.hdb 2>> temp_vg.out
$RUN sources temp_vg.hdb 2>> temp_vg.out
$RUN histogram temp_vg.hdb 2>> temp_vg.out
$RUN duplicates temp_vg.hdb 1 2>> temp_vg.out
$RUN hash_table temp_vg.hdb "1122334455667788" 2>> temp_vg.out
$RUN read_media temp_vg_hashes.txt 5 5 2>> temp_vg.out
$RUN read_media_size temp_vg_hashes.txt 2>> temp_vg.out

# Performance analysis
$RUN add_random temp_vg.hdb 100 2>> temp_vg.out
$RUN scan_random temp_vg.hdb 100 2>> temp_vg.out
$RUN add_same temp_vg.hdb 100 2>> temp_vg.out
$RUN scan_same temp_vg.hdb 100 2>> temp_vg.out

