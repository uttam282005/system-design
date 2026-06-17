## 1. The Core Purpose of Cache Eviction

Distributed caches (e.g., Redis, Memcached) use high-speed volatile storage mediums like system RAM to minimize access latencies. Because RAM is substantially more expensive than persistent disk storage, a cluster's cache capacity is inherently limited.

When a cache fills to its absolute threshold and a new record needs to be stored, the system must execute an **Eviction Policy** to eject an existing record from memory. The overarching goal of any eviction strategy is to systematically predict which keys are least likely to be queried in the future, maximizing the cluster's long-term **Cache Hit Ratio** and shielding downstream databases from un-cached query traffic.

---

## 2. Policy 1: First-In, First-Out (FIFO)

The **First-In, First-Out (FIFO)** strategy treats memory allocation as a strict time-based linear continuum. It operates under a simple chronological heuristic: the data that has resided in memory the longest should be the first to be discarded.

### Implementation Blueprint

FIFO is natively implemented using a standard **Queue** structure managed via a **Singly Linked List**.

* **Write Path ($O(1)$):** New elements entering the cache are appended directly to the tail of the list.
* **Eviction Path ($O(1)$):** When storage limits are breached, the system removes the element sitting at the head of the list, adjusts the head reference, and frees up space.

### The System Failure State: The Recency Blind Spot

FIFO does not track how often or how recently a key is accessed after insertion.

* If a critical key is queried millions of times a day but was inserted early in the cluster’s boot lifecycle, FIFO remains completely blind to its popularity.
* As unrelated entries arrive at the tail of the queue, the frequently accessed key is pushed to the head and evicted, triggering a wave of expensive cache misses.

---

## 3. Policy 2: Least Recently Used (LRU)

The **Least Recently Used (LRU)** policy solves FIFO's blind spot by introducing **recency tracking**. It operates on the principle of temporal locality: keys accessed recently are highly likely to be accessed again in the near future, whereas keys that have sat untouched for long durations are prime candidates for eviction.

### Implementation Blueprint: Hash Map + Doubly Linked List

To perform both lookups and position updates in constant time ($O(1)$), an LRU cache pairs a **Hash Map** with a **Doubly Linked List**.

* **The Hash Map:** Maps a target Key to the exact memory address pointer of its corresponding node within the Doubly Linked List, ensuring constant-time ($O(1)$) lookups.
* **The Doubly Linked List:** Establishes a strict ordering of recency. The head of the list represents the absolute newest (most recently accessed) item, while the tail represents the oldest. A *Doubly* Linked List is mandatory here because it allows nodes to slice themselves out of the middle of the chain in $O(1)$ time by manipulating their local `prev` and `next` pointers.

### Operational Lifecycle

1. **Cache Miss/Write:** A new key arrives. It is appended directly to the head of the list, and a corresponding key-pointer reference is inserted into the Hash Map. The system then drops the node sitting at the tail of the list and deletes its key from the Hash Map.
2. **Cache Hit/Read:** A user queries an existing key. The engine looks up the node pointer via the Hash Map. Using that pointer, it detaches the node from its current position in the middle of the list, splices the surrounding list connections together, and shifts the target node directly to the head of the list.

### Trade-offs

* **Pros:** Highly performant and structurally predictable. It handles dynamic reading spikes elegantly by continuously shifting active keys to the safe head zone of the list.
* **Cons:** Vulnerable to **Cache Pollution Scan Exploits**. If an analytical cron job or batch pipeline executes a broad sequential scan across a massive dataset, it will pull millions of one-off, low-priority keys into the cache. This forces the LRU mechanism to shift these useless keys to the head of the list while wiping out highly popular keys from memory.

---

## 4. Policy 3: Least Frequently Used (LFU)

The **Least Frequently Used (LFU)** policy focuses entirely on **access density** rather than raw timing. It tracks the cumulative popularity of a key over its lifespan, prioritizing the preservation of high-volume entries and evicting items with the lowest overall request counters.

### Implementation Blueprint

Implementing LFU in optimal $O(1)$ time is complex, typically requiring an infrastructure of **nested Doubly Linked Lists combined with dual Hash Maps**.

```
[Frequency Hash Map]         [Frequency Nodes Linked List]
   Key "A" ──► Counter: 8 ───► [Freq: 8] ──► List: [Node A] ◄──► [Node D]
   Key "B" ──► Counter: 4 ───► [Freq: 6] ──► List: [Node C]
   Key "C" ──► Counter: 6 ───► [Freq: 4] ──► List: [Node B] 
                                  │
                                  ▼ (Eviction targets the lowest frequency tail)

```

* **The Frequency List:** A primary doubly linked list tracks active frequency counter blocks ordered from highest to lowest.
* **The Data Nodes:** Each frequency block points to an internal, secondary doubly linked list containing all data nodes that share that exact access count.
* **The Lookup Maps:** Maintain immediate pointer access to both the data nodes and their parent frequency blocks.

### Operational Lifecycle

* **On Access Hit:** The system locates the data node, increments its internal counter, moves it out of its current frequency list, and appends it to the next frequency tier up (e.g., migrating from the frequency 7 list to the frequency 8 list).
* **On Eviction:** The engine targets the absolute lowest frequency bucket available at the tail of the main list and drops the oldest data node sitting within that specific sub-list.

### Trade-offs

* **Pros:** Immune to one-off data scan pollution bugs. A sudden bulk scan of unique keys will not overwrite historical, high-frequency core data.
* **Cons:** High memory overhead due to tracking frequency counters across all keys. It is also vulnerable to **Stale Historical Accumulation**. A specific key may experience massive popularity for one hour (e.g., a viral ticket sale event), driving its counter to millions. Once the event ends, the key may never be accessed again, yet its massive counter locks it inside the cache permanently, wasting valuable memory.

---

## 5. Architectural Comparison Matrix

| Eviction Policy | Internal Data Structures | Write/Evict Complexity | Read Hit Complexity | Primary Vulnerability |
| --- | --- | --- | --- | --- |
| **FIFO** | Singly Linked List + Queue | $O(1)$ | $O(1)$ | Ejects highly popular old data. |
| **LRU** | Hash Map + Doubly Linked List | $O(1)$ | $O(1)$ | Vulnerable to full-table scan pollution. |
| **LFU** | Dual Hash Maps + Nested Doubly Linked Lists | $O(1)$ | $O(1)$ | Accumulates obsolete historical high-frequency keys. |
| **Random** | Native Array / Hash Structure | $O(1)$ | $O(1)$ | Poor cache hit ratio predictability. |

### Architectural Edge-Case: Random Eviction

While **Random Eviction** provides poor hit-ratio optimization, it has one key advantage: **Zero Metadata Tracking Overhead**.

Policies like LRU and LFU require extra bytes per node to store pointers and counter metadata. In environments where memory is extremely constrained, opting for Random Eviction can save significant storage overhead at the cost of hit-ratio efficiency.
