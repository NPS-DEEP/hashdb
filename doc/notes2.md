# hashdb Data store

## Data Types
* `id_offset_pairs_t: typedef vector<pair<source_id, file_offset>> id_offset_pairs_t`
* class `hash_data_t {binary_hash, file_offset, entropy_label}`
*   or: class `hash_data_t {binary_hash, file_offset, block_entropy, block_type}`
* `hash_data_list_t: typedef vector<hash_data_t> hash_data_list_t`
* class `source_data_t {file_binary_hash, filesize, positive_count}`
*   or: class `source_data_t {file_binary_hash, filesize, positive_count, file_type}`
* `source_name_t: typedef pair<repository_name, fillename> source_name_t`
* `source_names_t: typedef vector<source_name_t> source_names_t`
* `file_mode_t {READ_ONLY, RW_NEW, RW_MODIFY}`

## LMDB Managers

### LMDB Hash Manager
`key=binary_hash, data=(source_id, file_offset)`

  or `key=binary_hash, data=(block_entropy, block_type)` and no LMDB Hash Label Manager.

* `lmdb_hash_manager_t(hashdb_dir, file_mode)`
* `void insert(source_id, hash_data_t, hashdb_changes_t&)`
* `void find(binary_hash, id_offset_pairs_t&)`
* `bool find(binary_hash)` - used for whitelist scan and to rebuild Bloom
* `pair(bool, binary_hash) find_begin(id_offset_pairs_t&)`
* `pair(bool, binary_hash) find_next(last_binary_hash, id_offset_pairs_t&)`
* `size_t size()`

### LMDB Hash Label Manager
`key=binary_hash, data=label_string`

* `lmdb_hash_label_manager_t(hashdb_dir, file_mode)`
* `void insert(binary_hash, label_string)`
* `string find(binary_hash)`  May return `""`.
* `size_t size()`

### LMDB Source File Binary Hash Manager
Look up source ID from file binary hash.

`key=file_binary_hash, data=source_id`

* `lmdb_source_file_binary_hash_manager_t(hashdb_dir, file_mode)`
* `pair(bool, source_id) insert(file_hash_id)` - true if inserted
* `pair(bool, source_id) find(file_hash_id)` - true if found
* `size_t size()`

### LMDB Source Data Manager
Look up source data from source ID.

`key=source_id, data=source_data_t`

* `lmdb_source_data_manager_t(hashdb_dir, file_mode)`
* `bool insert(source_id, file_binary_hash, filesize, positive_count)` - false if already there, fail if there and different.
* `source_data_t find(source_id)`
* `pair(source_id, source_data_t) find_begin()` - false if empty
* `pair(source_id, source_data_t) find_begin()` - fail if already at end
* `size_t size()`

### LMDB Source Name Manager
Look up a vector of source names given a source ID.

`key=source_id, data=(source_name_t)`.

* `lmdb_source_name_manager_t(hashdb_dir, file_mode)`
* `bool insert(source_id, repository_name, filename)` - true if already there
* `void find(source_id, source_names_t&)` - fail on invalid source ID
* `size_t size()`

## HASHDB Interfaces
hashdb interfaces use the `hashdb` namespace, are defined in `hashdb.hpp`, and are linked using the `libhashdb` library.  Interfaces can assert on unexpected error.

### Import `import_manager_t`
Import hashes.  All interfaces use lock.  Destructor appends changes to log.

* `import_manager_t(hashdb_dir, whitelist_hashdb_dir="", skip_low_entropy=false, cmd)`
* `pair(bool, source_id) import_file_binary_hash(file_binary_hash)` - false if already there
* `bool import_source_name(source_id, repository_name, filename)` - false if already there
* `bool import_source_data(source_id, source_data_t)` - false if there, fail if different
* `void import_hash(source_id, hash_data_t)` - changes are tracked
* `void import_hashes(source_id, hash_data_list_t)` - changes are tracked
* `string size()` - return sizes of LMDB databases
* `~import_manager_t()` - append changes to `hashdb_dir/log.dat`

### Scan `scan_manager_t`
* `scan_manager_t(hashdb_dir)`
* `void find_id_offset_pairs(binary_hash, id_offset_pairs_t&)`
* `void find_source_data(source_id, source_data_t&)`
* `void find_source_names(source_id, source_names_t&)`
* `pair(bool, source_id) find_source_id(file_binary_hash)`
* `pair(bool, binary_hash) hash_begin(id_offset_pairs&)`
* `pair(bool, binary_hash) hash_next(last_binary_hash, id_offset_pairs&)`
* `pair(file_binary_hash, source_data_t&) source_begin()`
* `pair(file_binary_hash, source_data_t&) source_next(last_file_binary_hash)`
* `string size()` - return sizes of LMDB databases

### Functions
* `extern C char* version()` - Return the hashdb version.
* `pair(bool, string) is_valid_hashdb` - Return true and "" else false and reason.
* `pair(bool, string) create_hashdb(hashdb_dir, setting parameters)` - Create a hash database given settings.  Return true and "" else false and reason.
* `pair(bool, string) hashdb_settings(hashdb_dir, setting parameters&)` - Query settings else false and reason.
* `pair(bool, string) rebuild_bloom(hashdb_dir, bloom filter parameters)` - Rebuild the Bloom filter.  Return true and "" else false and reason.
* `print_enviornment(command_line_string, ostream&)` - print command and environment to stream.


