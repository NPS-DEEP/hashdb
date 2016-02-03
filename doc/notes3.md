# hashdb Data store

## Data Types
* `id_offset_pair_t: typedef pair<source_id, file_offset> id_offset_pair_t`
* `id_offset_pairs_t: typedef set<id_offset_pair_t> id_offset_pairs_t`
* `source_name_t: typedef pair<repository_name, fillename> source_name_t`
* `source_names_t: typedef set<source_name_t> source_names_t`
* `file_mode_t {READ_ONLY, RW_NEW, RW_MODIFY}`

## LMDB Data Store Managers
The LMDB managers provide low-level interfaces used by the hashdb library and are not visible at the hashdb interface.

### LMDB Hash Manager
See if a block hash is likely to be present, find probable approximate source count.

`key=first 8 bytes of binary hash, data=set of last 8 bytes of binary hash matching the hash prefix in the key.`

* `lmdb_hash_manager_t(hashdb_dir, file_mode)`
* `void insert(binary_hash, &changes)`
* `size_t find(binary_hash)`
* `size_t size()`

### LMDB Hash Data Manager
Look up information associated with a given block hash.

`key=binary_hash, data=(entropy, block_label, set(source_id, file_offset))`

* `lmdb_hash_data_manager_t(hashdb_dir, file_mode)`
* `void insert(source_id, file_offset, binary_hash, entropy, block_label, &changes)` - change if different
* `bool find(binary_hash, entropy&, block_label&, id_offset_pairs_t&)`
* `pair(bool, binary_hash) find_begin()`
* `pair(bool, binary_hash) find_next(last_binary_hash)`
* `size_t size()`

### LMDB Source ID Manager
Look up source ID from file binary hash.

`key=file_binary_hash, data=source_id`

* `lmdb_source_id_hash_manager_t(hashdb_dir, file_mode)`
* `pair(bool, source_id, &changes) insert(file_binary_hash)` - true if a new source ID is created, false if source ID already exists
* `pair(bool, source_id) find(file_binary_hash)` - true if found else false and zero
* `size_t size()`

### LMDB Source Data Manager
Look up source data from source ID.

`key=source_id, data=(file_binary_hash, filesize, file_type, low_entropy_count)`

* `lmdb_source_data_manager_t(hashdb_dir, file_mode)`
* `void insert(source_id, file_binary_hash, filesize, file_type, low_entropy_count, &changes)` - change if different
* `bool find(source_id, file_binary_hash&, filesize&, file_type&, low_entropy_count&)` - false on no source ID
* `pair(bool, source_id) find_begin()` - false if empty
* `pair(bool, source_id) find_next()` - fail if already at end
* `size_t size()`

### LMDB Source Name Manager
Look up a set of source names given a source ID.

`key=source_id, data=(source_names_t)`.

* `lmdb_source_name_manager_t(hashdb_dir, file_mode)`
* `void insert(source_id, repository_name, filename, &changes)`
* `bool find(source_id, source_names_t&)` - false on no source ID
* `size_t size()`

## HASHDB Interfaces
hashdb interfaces use the `hashdb` namespace, are defined in `hashdb.hpp`, and are linked using the `libhashdb` library.  Interfaces can assert on unexpected error.

### Import
Import hashes.  Interfaces use lock for DB safety.  Destructor appends changes to change log.

* `import_manager_t(hashdb_dir)`
* `pair(bool, source_id) insert_source_id(file_binary_hash)` - false if already there
* `void insert_source_name(source_id, repository_name, filename)`
* `void insert_source_data(source_id, file_binary_hash, filesize, file_type, low_entropy_count)` - change if different
* `void insert_hash(binary_hash, source_id, file_offset, entropy, block_label)` - change if different
* `string size()` - return sizes of LMDB databases
* `~import_manager_t()` - append changes to change log at `hashdb_dir/log.dat`

### Scan
* `scan_manager_t(hashdb_dir)`
* `bool find_expanded_hash(binary_hash, expanded_text&)` - find and return JSON text
* `bool find_hash(binary_hash, entropy&, block_label&, id_offset_pairs_t&)`
* `size_t find_approximate_hash_count(binary_hash)`
* `bool find_source_data(source_id, file_binary_hash&, filesize&, file_type&, low_entropy_count&)` - false on no source ID
* `bool find_source_names(source_id, source_names_t&)` - false on no source ID
* `pair(bool, source_id) find_source_id(file_binary_hash)`
* `pair(bool, binary_hash) hash_begin()`
* `pair(bool, binary_hash) hash_next(last_binary_hash)`
* `pair(bool, source_id) source_begin()`
* `pair(bool, source_id) source_next(last_source_id)`
* `string sizes()` - return sizes of LMDB databases
* `size_t size()` - return number of unique hashes in hash_data

### Settings
Hold hashdb settings.

* `settings_t()` - create a settings object with default values.

### Timestamp
* `timestamp_t()`
* `string stamp(text)`
### Support Functions

* `extern C char* version()` - Return the hashdb version.
* `pair(bool, string) create_hashdb(hashdb_dir, settings)` - Create a hash database given settings.  Return true and "" else false and reason for failure.
* `pair(bool, string) read_settings(hashdb_dir, settings&)` - Query settings else false and reason for failure.
* `print_enviornment(command_line_string, ostream&)` - print command and environment to output stream. - this interface may be removed.
* `import_recursive_dir(hashdb_dir, whitelist_dir, min_entropy, max_entropy, repository_name, import_path)` - import from path to hashdb

