## 1. Distributed Caching Recap

A caching layer acts as a protective barrier or "wall" between users and the primary database, substantially lowering database load.

### Advantages

* **Faster Reads:** Data is physically closer to the user or stored in faster storage media (like RAM instead of disk).
* **Potential Write Benefits:** Depending on the pattern chosen, caching can speed up write operations.

### Disadvantages & Challenges

* **Expensive Cache Misses:** A cache miss introduces extra network overhead and lookup time. While a hashmap-based cache provides constant time complexity $O(1)$, some caches use a tree map structure, which introduces logarithmic time complexity $O(\log n)$ and increases CPU/memory utilization.
* **Data Consistency:** Managing updates across multiple data stores complicates correctness and consistency.

---

## 2. Distributed Cache Write Strategies

When writing data in a system with a cache, there are three primary design patterns, each striking a different balance between complexity, speed, and consistency.

---

### Strategy A: Write-Around Cache

In this approach, the application completely bypasses the cache during a write operation and writes the updated data directly to the database. The database serves as the sole source of truth.

To deal with the fact that the cache now holds old data, systems handle reads in one of two ways:

#### 1. Stale Reads with Time-To-Live (TTL)

* **How it works:** The old data remains inside the cache until its predetermined TTL expiration timestamp passes.
* **The Flow:** The application reads old data from the cache until it expires. Once expired, the next read triggers a cache miss, pulls the new data from the database, and repopulates the cache.
* **Trade-off:** High data staleness; simple to implement.

#### 2. Cache Invalidation

* **How it works:** Immediately after writing the updated value to the database, the application actively deletes or marks the corresponding key as invalid in the cache.
* **The Flow:** The next read request hits an empty cache entry (cache miss), fetches the fresh record from the database, saves it back into the cache, and returns it to the user.
* **Trade-off:** Stronger data consistency than TTL, but introduces an immediate, expensive cache miss penalty for the subsequent read.

#### Overall Summary

* **Pros:** Simple write path; safe architecture because the database is updated directly. Adding read-only caches won't break your existing write infrastructure.
* **Cons:** High read latency immediately following an update due to mandatory cache misses.

---

### Strategy B: Write-Through Cache

This pattern routes all write requests directly to the cache. The cache acts as a proxy, taking responsibility for immediately intercepting the write and passing it along to the primary database before confirming a success back to the user.

#### Implementation Approaches & Consistencies

* **The Strict Approach (Two-Phase Commit / 2PC):** To guarantee absolute synchronization, the system uses a two-phase commit protocol. The application initiates a write, both the cache and database prepare the write, confirm readiness, and execute the commit simultaneously.
* *Downside:* Mechanically complex and very slow due to the high volume of back-and-forth network coordination calls.


* **The Asynchronous / "YOLO" Approach:** The cache writes to its own storage and immediately sends a network request to update the database without tightly enforcing a two-phase agreement.
* *Downside:* Risks serious data drift. If the cache updates successfully but the network call to the database fails, or if the cache crashes before completing the database sync, the two layers fall permanently out of step.



#### Overall Summary

* **Pros:** Excellent data consistency (when strictly paired with 2PC) because data lives concurrently in both storage layers. Up-to-date data is instantly warm in the cache for subsequent reads.
* **Cons:** Slower write operations since every write requires writing to multiple distinct locations over the network.

---

### Strategy C: Write-Back Cache (Write-Behind)

The application writes data exclusively to the low-latency cache layer, which acknowledges success immediately. The cache then saves the updates back to the database at a later time asynchronously.

#### Key Optimization: Mini-Batching

To save on expensive network and disk I/O, write-back caches often aggregate multiple discrete write operations over a short window. It groups these changes together into a single "mini-batch" push to the database, radically reducing transaction load.

#### Handling Extreme Data Consistency (The Distributed Lock Method)

If a system using a write-back cache absolutely must serve consistent data to a concurrent reader trying to query the database directly, it requires a complex workaround:

1. The writing application acquires a cluster-wide distributed lock on the data key.
2. If another process attempts to read that key from the database while the lock is active, the distributed locking service intercepts the request.
3. The locking mechanism alerts the write-back cache, forcing it to immediately flush its pending changes down to the database.
4. Once written, the lock releases, and the reader securely pulls the fresh data from the database.

*Note:* While this architecture ensures correctness, it introduces immense code complexity and latency penalties, which fundamentally defeats the primary speed benefits of selecting a write-back pattern in the first place.

#### Overall Summary

* **Pros:** Extremely fast write performance because operations hit low-latency memory targets without blocking on database commits. Ideal for applications that can tolerate temporary data staleness.
* **Cons:** Highly vulnerable to data loss. If the cache node suffers a hardware failure, power cut, or crash before its memory buffers are flushed to the database, that unwritten data is permanently destroyed.

---

## 3. High-Level Trade-Off Matrix

| Strategy | Write Target | Write Latency | Read Latency (Post-Write) | Primary Risk | Best Use Case |
| --- | --- | --- | --- | --- | --- |
| **Write-Around** | Database | Normal | High (Cache Miss / Invalidation) | Serving stale data (if using TTL) | Systems with infrequent updates or where read delays are acceptable. |
| **Write-Through** | Cache $\rightarrow$ Database | High | Low (Cache Warm) | Complex 2PC overhead or out-of-sync data on network drops | Applications requiring highly concurrent, immediately consistent reads. |
| **Write-Back** | Cache | Low | Low | Permanent data loss if the cache node crashes | High-throughput write systems where absolute speed trumps durability. |
|  |  |  |  |  |  |
