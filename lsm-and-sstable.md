### **1. Core Concept of LSM Trees**
An LSM (Log-Structured Merge) tree is a type of database index designed to handle high write volumes. The primary component is an **in-memory balanced binary search tree** (such as a Red-Black tree, AVL tree, or B-tree).
*   Because operations happen in memory, both reads and writes are highly performant with an **O(log n)** time complexity.

### **2. Overcoming In-Memory Limitations**
Relying solely on memory introduces two major challenges, which the LSM tree architecture elegantly solves:
*   **Durability:** Data in memory is lost if the machine crashes. 
    *   *Solution:* A **Write-Ahead Log (WAL)** is used. This is a sequentially written log stored on disk. If the system crashes, the WAL is replayed to rebuild the in-memory tree. Since writes are sequential, this maintains relatively fast write performance, though it introduces some disk I/O.
*   **Limited Space:** Ram is limited, and not all database keys can fit in memory.
    *   *Solution:* **SS Tables** (Sorted String Tables). Once the in-memory tree surpasses a specific size threshold, it is purged from memory and converted into an SS Table file on disk. 

### **3. SS Tables (Sorted String Tables)**
An SS table is an **immutable, sorted list** of keys and values saved to disk.
*   **Efficient Creation:** Because the in-memory structure is already a binary search tree, the system can perform an in-order traversal in linear time **(O(n))** to quickly convert the tree into a sorted list for the SS table. 
*   Over time, multiple SS tables will accumulate on disk as the in-memory tree continuously fills up and gets flushed.

### **4. Basic Database Operations**
*   **Writes:** All new writes are directed to the in-memory LSM tree.
*   **Updates and Deletes:** Because SS tables on disk are immutable, you cannot overwrite existing data. To update a value, you simply write the new value to the in-memory tree; since the system prioritizes newer data, it will naturally supersede the old value. To delete a value, you write a **"Tombstone"** marker (a special deletion flag) to indicate that the key has been removed.
*   **Reads:** To find a key, the system searches in a specific reverse-chronological order:
    1. First, it searches the in-memory LSM tree.
    2. If not found, it checks the most recent SS table on disk.
    3. It continues checking older SS tables in order until the key (or a tombstone marker) is found.
    *   Since SS tables are sorted, they are queried using a binary search, resulting in an O(log n) lookup complexity per file. 

### **5. LSM Tree Optimizations**
Because reads potentially require searching through multiple files on disk, the system uses a few clever optimizations:
*   **Sparse Index:** Instead of tracking every single key on disk, the database creates an index containing only a subset of keys and their exact locations in the SS table. This allows the database to narrow down the start and end points of a binary search, skipping unnecessary iterations and speeding up the read process.
*   **Bloom Filters:** A probabilistic data structure that can tell you if a key is *not* in a table. If the bloom filter confirms a key is missing, the system can entirely skip searching that specific SS table. 
*   **Compaction (Space Optimization):** Because updates and tombstones create duplicate keys across different tables, storage space is wasted over time. Compaction is a background process that merges older SS tables into a single new one, keeping only the most recent values and discarding duplicates and tombstones. This is done using a two-pointer approach to merge sorted arrays in **O(n)** time.

### **6. Index Comparisons & Trade-offs**
The video concludes by comparing LSM trees to Hash Indexes and B-Trees:
*   **Hash Index:** Offers extremely fast **O(1)** reads and writes in memory. However, all keys must fit in RAM, and it cannot support range queries.
*   **B-Tree:** The standard index for many relational databases. It handles big datasets well (stored on disk) and excels at range queries because similar keys are co-located physically on the disk. However, writes are slower because they have to jump around the disk to write directly to the tree structure.
*   **LSM Tree:** Serves as a middle ground. Unlike a hash index, it is not limited by RAM size. Writes are faster than B-Trees because they initially hit memory (despite the WAL overhead). It supports range queries because the SS tables are sorted, though these queries (and normal reads) can be slower than a B-Tree since they must span multiple files. Finally, it relies on background CPU usage for regular compaction.
