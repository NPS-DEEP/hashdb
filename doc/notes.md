# hashdb Data store

## LMDB Data Types `lmdb_typedefs.h`
* `typedef vector<pair<source_id, file_offset>> id_offset_pairs_t`
* `typedef pair<binary_hash, id_offset_pairs_t> hashdb_scan_it_data_t`
* class `source_metadata_t {file_binary_hash, source_id, filesize, positive_count}`
* class `hash_data_t {binary_hash, file_offset, entropy_label=""}`
* `typedef vector<hash_data_t> hash_data_list_t`
* `typedef pair<repository_name, fillename> source_name_t`
* `typedef vector<source_name_t> source_names_t`

## hashdb Databases
* `hashes` - LMDB multimap of `key=binary_hash, value=(source ID, source offset)`.
* `hash_labels` - LMDB map of `key=binary_hash, value=label` for keys where labels are produced.
* `source_ids` - LMDB map of `key=source_id, value=file_binary_hash`.
* `source_metadata` - LMDB map of `key=file_binary_hash, value=(source_id, filesize, positive_count)`.
* `source_names` - LMDB multimap of `key=file_binary_hash, value=(repository_name, filename)`.

## LMDB Managers

* file_mode: `READ_ONLY, RW_NEW, RW_MODIFY`

### LMDB Hash Manager `lmdb_hash_manager_t`

* `lmdb_hash_manager_t(hashdb_dir, file_mode)` - reads `settings.json` file
* `void insert(source_id, hash_data_list_t, hashdb_changes_t)` - updates changes
* `void find(binary_hash, id_offset_pairs_t&)`
* `binary_hash find_begin(id_offset_pairs_t&)`
* `binary_hash find_next(last_binary_hash, id_offset_pairs_t&)`
* `size_t size()`

### LMDB Hash Label Manager

* `lmdb_hash_label_manager_t(hashdb_dir, file_mode)`
* `bool insert(binary_hash, label_string)`
* `string find(binary_hash)`  May return `""`.
* `size_t size()`

### LMDB Source ID Manager
Look up file hash from source ID when scanning.

* `lmdb_source_id_manager_t(hashdb_dir, file_mode)`
* `void insert(source_id, file_binary_hash)` - warn to stderr and do not insert if already there
* `file_binary_hash find(source_id)` - fail if not there
* `size_t size()`

### LMDB Source Metadata Manager
Look up source metadata from source hash when scanning.
Use two-step import when importing vector of hashes from a source.

* `lmdb_source_metadata_manager_t(hashdb_dir, file_mode)`
* `pair(bool, source_id) insert_begin(file_binary_hash)` - true if ready to begin importing block hashes, false if block hashes have already been imported for this source
* `void insert_end(file_binary_hash, source_id, filesize, positive_count)` - fail if not already there, warn to stderr and do not insert if already there and filesize not zero
* `source_metadata_t find(file_binary_hash)` - fail if not there
* `source_metadata_t find_begin()` - `file_binary_hash` is `""` if empty
* `source_metadata_t find_next(last_file_binary_hash)` - fail if already at end
* `size_t size()`

### LMDB Source Name Manager
Look up `source_names_t` vector of repository name, filename pairs from file hash.

* `lmdb_source_name_manager_t(hashdb_dir, file_mode)`
* `void insert(file_binary_hash, repository_name, filename)` - okay if already there, but do not re-add
* `void find(file_binary_hash, &source_names_t)` - return empty vector if not there
* `size_t size()`


## HASHDB Managers
### HASHDB Import `hashdb_import_manager_t`

* `hashdb_import_manager_t(hashdb_dir, whitelist_hashdb_dir="", import_low_entropy=false)`
* `bool has_file_hash(file_binary_hash)` - used to detect if this file has already been imported.
* `import_hashes(file_binary_hash, repository_name, filename, file_size, hash_data_list_t)` - Used to import all interesting hashes for a new file hash.  For DB safety: lock, call `has_file_hash`, import the vector, set `has_file_hash`, then unlock.  `entropy_label` not used.
* `import_alternate_source(file_binary_hash, repository_name, filename)` - used to map another filename to this file hash.
* `json_string size()`
* `~hashdb_import_manager_t()` - append change log from `changes_t` to `hashdb_dir/log.dat`

### HASHDB Scan `hashdb_scan_manager_t`
* `hashdb_scan_manager_t(hashdb_dir, out_path)`
* `id_offset_pairs_t find(binary_hash)`
* `source_names_t find_source_names(file_binary_hash)`
* `hashdb_scan_sizes_t size()`
* `binary_hash hash_begin()`
* `binary_hash hash_next(last_binary_hash)`
* `source_metadata_t source_begin()`
* `source_metadata_t source_next(last_binary_file_hash)`
* `source_names_t source_names(binary_file_hash)`
* `json_string size()`


