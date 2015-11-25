# hashdb Data store
## DB Managers
### LMDB Hash Manager `lmdb_hash_manager_t`

Use `typedef vector<pair<source_id, file_offset>> id_offset_pairs_t`

* `lmdb_hash_manager_t(hashdb_dir, rw_mode, sector_size, hash_truncation)`
* `bool insert(hash, source_id, file_offset)`
* `id_offset_pairs_t find(binary_hash)`
* `size_t size()`
* `lmdb_hash_pointer_t begin()`
* `lmdb_hash_pointer_t end()`

### LMDB Hash Iterator `lmdb_hash_it_t`
* `*` Dereferencing returns `pair<binary_hash, id_offset_pairs_t>`
* `++` increments iterator to next hash value.
* As with any C++ iterator, do not dereference end or increment past end.

### LMDB Hash Label Manager

* `lmdb_hash_label_manager_t(hashdb_dir, rw_mode)`
* `bool insert(binary_hash, label_string)`
* `string find(binary_hash)`  May return "".
* `size_t size()`

### SQL Source Manager `sql_source_manager_t`

Use type `source_metadata_t<source_id, filesize, import_count>`

Use `typedef vector<pair<repository_name, filename>> source_names_t`

* `sql_source_manager_t(hashdb_dir, rw_mode)`

Source ID to file hash:

* `bool source_id_to_file_hash_insert(source_id, file_binary_hash)`
* `pair<bool, file_binary_hash> source_id_to_file_hash_find(source_id)`
* `size_t source_id_to_file_hash_size()`

Source metadata:

* bool source_metadata_insert(file_binary_hash, source_metadata)
* source_metadata find(file_binary_hash)
* `size_t source_metadata_size()`
* `source_names_t source_metadata_find(file_binary_hash)`
* `size_t source_metadata_size()`

Source name lookup:

* `source_name_insert(file_binary_hash, repository_name, filename)`
* `source_names_t source_name_find(file_binary_hash)`
* `size_t source_name_size()`

Source iterator:

* `begin()`
* `end()`

### SQL Source Iterator `sql_source_it_t`
* `*` Dereferencing returns `sql_source_it_data_t`
* `++` increments iterator to next hash value.

Use type: `sql_source_it_data_t<file_binary_hash, source_metadata, source_names>`

## HASHDB DB Managers
### HASHDB Import `hashdb_import_t`

* `hashdb_import_t(hashdb_dir)`
* `bool has_file_hash(file_MD5)` - used to detect if this file has already been imported.
* `import_hashes(file_md5, repository_name, filename, file_size, vector(file_offset, block_hash, label="")` - used to import all interesting hashes for a new file_md5.
* `import_alternate_source(file_md5, repository_name, filename)` - used to map another filename to this file hash.

### HASHDB Scan
* `hashdb_scan_t(hashdb_dir)`
* `id_offset_pairs_t find(binary_hash)`
* `hashdb_scan_sizes_t size()`
* `begin()`
* `end()`

### HASHDB Scan Iterator `hashdb_scan_it_t`
Use `typedef pair<binary_hash, id_offset_pairs_t> hashdb_scan_it_data_t`

* `*` Dereferencing returns `hashdb_scan_it_data_t`
* `++` increments iterator to next hash value.




