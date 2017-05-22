                     Announcing hashdb 3.1.0
                       May 22, 2017

                          RELEASE NOTES

hashdb Version 3.1.0 has been released for Linux, MacOS and Windows.

## Changes
*hashdb* 3.1.0 provides a halting bug fix that manifests when building large databases. The halt is a result of heap space exhaustion due to excessive page allocations in LMDB resulting from increasing record sizes on existing records. *hashdb* 2.0.0 does not increase record size and does not manifest this bug. This bug results in program termination. The fix replaces changed records in-place without changing record size.  To accomplish this, we change the data store as follows:

* *hashdb* no longer stores source offset values in the hash data store.  These values were used to identify where a block hash is located within a source.  This information may be obtained by reexamining the source file.  As a result of this change, the byte alignment parameter seen when creating a new database is no longer required and is discontinued.
* *hashdb* no longer stores hash suffix values in a list in the hash store.  Instead, the hash prefix is longer and is hard-coded to 7 bytes.  In a database of one billion hashes, this will result in a false positive rate of one in 72 million.  Recall that the hash store is an approximate store and that complete hashes are stored in the hash data store.

## Compatibility
* This fix breaks compatibility with earlier versions of *hashdb*.  If your use case does not require a very large data store, this fix is not necessary.
* The removal of suffix values breaks compatibility with with *SectorScope* 0.7.0.  If you would like to use *SectorScope* with *hashdb* 3.1.0, please use *SectorScope* 0.7.1.
