if -%1-==-- echo usage: find_blocks file image & exit /b
@echo Using md5deep64 to generate temp1.xml from file %1 ...
md5deep64 -p 4096 -d %1 > temp1.xml
@echo Creating hash database temp1.hdb ...
hashdb create temp1.hdb
@echo Importing hashes from temp1.xml into temp1.hdb ...
hashdb import -r temp1 temp1.hdb temp1.xml
@echo Scanning image %2 for block hashes that match block hashes
@echo in temp1.hdb, putting result in %cd%\temp2 ...
bulk_extractor -e hashdb -S hashdb_mode=scan -S hashdb_scan_path_or_socket=temp1.hdb -o temp2 %cd%\%2
@echo Identifying sources from temp2\identified_blocks.txt ...
@echo # Forensic Path          hashdigest         repository name, filename, file offset > temp2\identified_sources.txt
hashdb expand_identified_blocks temp1.hdb temp2\identified_blocks.txt >> temp2\identified_sources.txt
@echo File temp2\identified_sources.txt is now ready.
@echo Done.
