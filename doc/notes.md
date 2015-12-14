# hashdb Data store

## LMDB Data Types `lmdb_typedefs.h`
* `typedef vector<pair<source_id, file_offset>> id_offset_pairs_t`
* `typedef pair<binary_hash, id_offset_pairs_t> hashdb_scan_it_data_t`
* class `source_metadata_t {source_id, filesize, positive_count}`
* class `hash_data_t {binary_hash, file_offset, entropy_label=""}` - a field may be added for other label types, but use `entropy_label` for `hashdb_import_manager_t` `skip_low_entropy` parameter
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

* `lmdb_hash_manager_t(hashdb_dir, file_mode)` - reads settings file
* `void insert(source_id, hash_data_t, hashdb_changes_t)` - updates changes
* `void find(binary_hash, id_offset_pairs_t&)`
* `void find(binary_hash)` - used for whitelist scan and to rebuild Bloom
* `binary_hash find_begin(id_offset_pairs_t&)`
* `binary_hash find_next(last_binary_hash, id_offset_pairs_t&)`
* `size_t size()`

### LMDB Hash Label Manager

* `lmdb_hash_label_manager_t(hashdb_dir, file_mode)`
* `void insert(binary_hash, label_string)`
* `string find(binary_hash)`  May return `""`.
* `size_t size()`

### LMDB Source ID Manager
Look up file hash from source ID when scanning.  `key=source_id, data=file_binary_hash`.

* `lmdb_source_id_manager_t(hashdb_dir, file_mode)`
* `void insert(source_id, file_binary_hash)` - warn to stderr and do not insert if already there
* `void file_binary_hash find(source_id)` - fail if not there
* `size_t size()`

### LMDB Source Metadata Manager
Look up source metadata from source hash when scanning.
Use two-step import when importing vector of hashes from a source.
`key=file_binary_hash, data=(source_id, filesize, positive_count)`.

* `lmdb_source_metadata_manager_t(hashdb_dir, file_mode)`
* `pair(bool, source_id) insert_start(file_binary_hash)` - true if ready to start importing block hashes, false if block hashes have already been imported for this source
* `void insert_stop(file_binary_hash, source_id, filesize, positive_count)` - fail if not already there, warn to stderr and do not insert if already there and filesize not zero
* `source_metadata_t find(file_binary_hash)` - fail if not there
* `pair(file_binary_hash, source_metadata_t) find_begin()` - `file_binary_hash` is `""` if empty
* `pair(file_binary_hash, source_metadata_t) find_next(last_file_binary_hash)` - fail if already at end
* `size_t size()`

### LMDB Source Name Manager
Look up `source_names_t` vector of repository name, filename pairs from file hash.
`key=source_id, data=file_binary_hash`.

* `lmdb_source_name_manager_t(hashdb_dir, file_mode)`
* `void insert(file_binary_hash, repository_name, filename)` - okay if already there, but do not re-add
* `void find(file_binary_hash, source_names_t&)` - return empty vector if not there
* `size_t size()`


## HASHDB Managers
### HASHDB Create `hashdb_create_manager`
* `bool hashdb_create_manager::create_if_new(hashdb_dir)` - false, true, or fail.
### HASHDB Import `hashdb_import_manager_t`
Import hashes.  All interfaces use lock.  Destructor appends to log.

* `hashdb_import_manager_t(hashdb_dir, whitelist_hashdb_dir="", skip_low_entropy=false)`
* `bool import_source_name(file_binary_hash, repository_name, filename)` - initialize the environment for this file hash.  Import name if new.  True: need to import block hashes.  False: block hashes for this source have already been imported.
* `void import_source_data(file_binary_hash, filesize, hash_data_list_t)` - import block hash data and source metadata for this source.  Fail if `import_source_name` not called first for this `file_binary_hash`.
* `string size()` - return sizes of LMDB databases
* `~hashdb_import_manager_t()` - append change log from `changes_t` to `hashdb_dir/log.dat`

### HASHDB Scan `hashdb_scan_manager_t`
* `hashdb_scan_manager_t(hashdb_dir)`
* `void find_id_offset_pairs(binary_hash, id_offset_pairs_t&)`
* `void find_source_names(file_binary_hash, source_names_t&)`
* `string find_file_binary_hash(source ID)`
* `binary_hash hash_begin(id_offset_pairs&)`
* `binary_hash hash_next(last_binary_hash, id_offset_pairs&)`
* `pair(file_binary_hash, source_metadata_t) source_begin()`
* `pair(file_binary_hash, source_metadata_t) source_next(last_file_binary_hash)`
* `string size()` - return sizes of LMDB databases


