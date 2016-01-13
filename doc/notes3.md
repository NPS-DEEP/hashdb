# hashdb Data store

## Data Types
* `id_offset_pair_t: typedef pair<source_id, file_offset> id_offset_pair_t`
* `id_offset_pairs_t: typedef set<id_offset_pair_t> id_offset_pairs_t`
* `source_name_t: typedef pair<repository_name, fillename> source_name_t`
* `source_names_t: typedef set<source_name_t> source_names_t`
* `file_mode_t {READ_ONLY, RW_NEW, RW_MODIFY}`

## LMDB Managers
The LMDB managers provide low-level interfaces used by the hashdb library and are not visible at the hashdb interface.

### LMDB Hash Manager
`key=first 8 bytes of binary hash, data=set of last 8 bytes of binary hash matching the hash prefix in the key.`

* `lmdb_hash_manager_t(hashdb_dir, file_mode)`
* `bool insert(binary_hash)` - false if already there
* `bool find(binary_hash)`
* `size_t size()`

### LMDB Hash Data Manager
`key=binary_hash, data=(low_entropy_label, entropy, block_label, set(source_id, file_offset))`

* `lmdb_hash_data_manager_t(hashdb_dir, file_mode)`
* `bool insert_hash_data(binary_hash, low_entropy_label, entropy, block_label)` - true if new, false, do not change, and note if not new
* `bool insert_hash_source(binary_hash, source_id, file_offset)` - false if source already there or more than hardcoded max, fail if invalid file offset or no hash data
* `void find(binary_hash, low_entropy_label&, entropy&, block_label&, id_offset_pairs_t&)`
* `pair(bool, binary_hash) find_begin()`
* `pair(bool, binary_hash) find_next(last_binary_hash)`
* `size_t size()`

### LMDB Source ID Manager
Look up source ID from file binary hash.

`key=file_binary_hash, data=source_id`

* `lmdb_source_id_hash_manager_t(hashdb_dir, file_mode)`
* `pair(bool, source_id) insert(file_binary_hash)` - true if a new source ID is created, false if source ID already exists
* `pair(bool, source_id) find(file_binary_hash)` - true if found else false and zero
* `size_t size()`

### LMDB Source Data Manager
Look up source data from source ID.

`key=source_id, data=(file_binary_hash, filesize, file_type, low_entropy_count)`

* `lmdb_source_data_manager_t(hashdb_dir, file_mode)`
* `bool insert(source_id, file_binary_hash, filesize, file_type, low_entropy_count)` - true if new, false, do not change, and note if not new
* `void find(source_id, file_binary_hash&, filesize&, file_type&, low_entropy_count&)` - fail on invalid source ID
* `pair(bool, source_id) find_begin()` - false if empty
* `pair(bool, source_id) find_next()` - fail if already at end
* `size_t size()`

### LMDB Source Name Manager
Look up a set of source names given a source ID.

`key=source_id, data=(source_names_t)`.

* `lmdb_source_name_manager_t(hashdb_dir, file_mode)`
* `bool insert(source_id, repository_name, filename)` - true if inserted, false if already there
* `void find(source_id, source_names_t&)` - fail on invalid source ID
* `size_t size()`

## HASHDB Interfaces
hashdb interfaces use the `hashdb` namespace, are defined in `hashdb.hpp`, and are linked using the `libhashdb` library.  Interfaces can assert on unexpected error.

### Import
Import hashes.  Interfaces use lock for DB safety.  Destructor appends changes to change log.

* `import_manager_t(hashdb_dir)`
* `pair(bool, source_id) insert_source_file_hash(file_binary_hash)` - false if already there
* `bool insert_source_name(source_id, repository_name, filename)` - false if already there
* `bool insert_source_data(source_id, file_binary_hash, filesize, file_type, low_entropy_count)` - true if new, false, do not change, and note if not new
* `bool insert_hash(binary_hash)` - true if inserted, false if already there
* `bool insert_hash_data(binary_hash, low_entropy_label, entropy, block_label)` - true if new, false, do not change, and note if not new
* `bool insert_hash_source(binary_hash, source_id, file_offset)` - false if source already there or more than hardcoded max, fail if invalid file offset or no hash data
* `string size()` - return sizes of LMDB databases
* `~import_manager_t()` - append changes to change log at `hashdb_dir/log.dat`

### Scan
* `scan_manager_t(hashdb_dir)`
* `bool find_hash(binary_hash)`
* `void find_hash_data(binary_hash, low_entropy_label&, entropy&, block_label&, id_offset_pairs_t&)`
* `void find_source_data(source_id, file_binary_hash&, filesize&, file_type&, low_entropy_count&)` - fail on invalid source ID
* `void find_source_names(source_id, source_names_t&)` - fail on invalid source ID
* `pair(bool, source_id) find_source_id(file_binary_hash)`
* `pair(bool, binary_hash) hash_begin()`
* `pair(bool, binary_hash) hash_next(last_binary_hash)`
* `pair(bool, source_id) source_begin()`
* `pair(bool, source_id) source_next(last_source_id)`
* `string size()` - return sizes of LMDB databases

### Functions
* `extern C char* version()` - Return the hashdb version.
* `pair(bool, string) is_valid_hashdb` - Return true and "" else false and reason the DB is not valid.
* `pair(bool, string) create_hashdb(hashdb_dir, setting parameters)` - Create a hash database given settings.  Return true and "" else false and reason for failure.
* `pair(bool, string) hashdb_settings(hashdb_dir, setting parameters&)` - Query settings else false and reason for failure.
* `print_enviornment(command_line_string, ostream&)` - print command and environment to output stream.
* `import_recursive_dir(hashdb_dir, whitelist_dir, min_entropy, max_entropy, repository_name, import_path)` - import from path to hashdb

