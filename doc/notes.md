# hashdb Data store

## db Types `db_typedefs.h`
* `typedef vector<pair<source_id, file_offset>> id_offset_pairs_t`
* `typedef pair<binary_hash, id_offset_pairs_t> hashdb_scan_it_data_t`
* `typedef vector<pair<repository_name, filename>> source_names_t`
* class `source_metadata_t {source_id, filesize, import_count}`
* class `hash_data_t {binary_hash, file_offset, entropy_label=""}`
* `typedef vector<hash_data_t> hash_data_list_t`
* class `sql_source_it_data_t {file_binary_hash, source_metadata, source_names}`

## hashdb Databases
* `hashes` - LMDB multimap of `key=binary_hash, value=(source ID, source offset)`.
* `hash_labels` - LMDB map of `key=binary_hash, value=label` for keys where labels are produced.
* `source_id_to_file_hash` - SQL lookup map of `key=source_id, value=file_hash`.
* `source_metadata` - SQL lookup of `key=file_hash, value=(source_id, filesize, import_count)`.
* `sources` - SQL lookup multimap of `key=file_hash, value=(repository_name, filename`.

## Bottom: DB Managers

* file_mode: `READ_ONLY, RW_NEW, RW_MODIFY`

### LMDB Hash Manager `lmdb_hash_manager_t`

* `lmdb_hash_manager_t(hashdb_dir, file_mode)` - reads `settings.json` file
* `void insert(source_id, hash_data_list_t, hashdb_changes_t)` - updates changes
* `id_offset_pairs_t find(binary_hash)`
* `size_t size()`
* `lmdb_hash_pointer_t begin()`
* `lmdb_hash_pointer_t end()`

### LMDB Hash Iterator `lmdb_hash_it_t`
* `*` Dereferencing returns `pair<binary_hash, id_offset_pairs_t>`
* `++` increments iterator to next hash value.
* As with any C++ iterator, do not dereference end or increment past end.

### LMDB Hash Label Manager

* `lmdb_hash_label_manager_t(hashdb_dir, file_mode)` - reads `settings.json` file
* `bool insert(binary_hash, label_string)`
* `string find(binary_hash)`  May return `""`.
* `size_t size()`

### SQL Source Manager `sql_source_manager_t`

* `sql_source_manager_t(hashdb_dir, file_mode)` - reads `settings.json` file

Source ID to file hash:

* `bool source_id_to_file_hash_insert(source_id, file_binary_hash)`
* `pair<bool, file_binary_hash> source_id_to_file_hash_find(source_id)`
* `size_t source_id_to_file_hash_size()`

Source metadata:

* `bool source_metadata_insert(file_binary_hash, source_metadata)`
* `source_metadata find(file_binary_hash)`
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

## Middle: HASHDB DB Managers
### HASHDB Import `hashdb_import_manager_t`

* `hashdb_import_manager_t(hashdb_dir, whitelist_hashdb_dir="", import_low_entropy=false)`
* `bool has_file_hash(file_hash)` - used to detect if this file has already been imported.
* `import_hashes(file_hash, repository_name, filename, file_size, hash_data_list_t)` - Used to import all interesting hashes for a new file hash.  For DB safety: lock, call `has_file_hash`, import the vector, set `has_file_hash`, then unlock.  `entropy_label` not used.
* `import_alternate_source(file_hash, repository_name, filename)` - used to map another filename to this file hash.
* `~hashdb_import_manager_t()` - append change log from `changes_t` to `hashdb_dir/log.dat`

### HASHDB Scan `hashdb_scan_manager_t`
* `hashdb_scan_manager_t(hashdb_dir, out_path, out_mode)` where mode is `"JSON"` or `"SQL"`
* `id_offset_pairs_t find(binary_hash)`
* `hashdb_scan_sizes_t size()`
* `begin()`
* `end()`

### HASHDB Scan Iterator `hashdb_scan_it_t`

* `*` Dereferencing returns `hashdb_scan_it_data_t`
* `++` increments iterator to next hash value.

