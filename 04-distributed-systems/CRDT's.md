# System Design Notes: Conflict-Free Replicated Data Types (CRDTs)

## 1. Introduction & Context

In multi-leader and leaderless replication environments, concurrent write conflicts are guaranteed to occur. When designing for resolution, engineers traditionally have three strategies [[00:29](http://www.youtube.com/watch?v=FG5Varj1Ows&t=29)]:

1. **Last Write Wins (LWW):** Fast, but highly unreliable due to physical clock skew and NTP synchronization variations [[00:44](http://www.youtube.com/watch?v=FG5Varj1Ows&t=44)].
2. **Sibling Storage (Application Intervention):** Tracks concurrency via version vectors and writes overlapping states simultaneously, offloading the resolution effort onto the user or application logic [[01:02](http://www.youtube.com/watch?v=FG5Varj1Ows&t=62)].
3. **Automatic Convergence via CRDTs:** Specialized abstract data types configured natively within the database engine that leverage deterministic properties to auto-merge incoming streams safely without external intervention [[01:12](http://www.youtube.com/watch?v=FG5Varj1Ows&t=72)].

* **Real-World Precedent:** Real-world implementations of CRDTs are visible in enterprise databases like **Riak** (distributed leaderless key-value engine) [[02:12](http://www.youtube.com/watch?v=FG5Varj1Ows&t=132)] and **Redis Enterprise** (which leverages CRDTs to manage conflict-free clusters of sets and maps directly in memory) [[02:24](http://www.youtube.com/watch?v=FG5Varj1Ows&t=144)].

---

## 2. Operational CRDTs vs. State-Based CRDTs

There are two architectural implementations for syncing changes across nodes using CRDT patterns.

### A. Operational CRDTs (Op-Based)

Instead of broadcasting entire state structures over wire networks, nodes transmit discrete **mutation operations** (e.g., sending the packet `Increment(Leader_0)`) [[03:34](http://www.youtube.com/watch?v=FG5Varj1Ows&t=214)].

* **Advantage:** Unbelievably lightweight. The wire payload stays fixed at an $O(1)$ spatial footprint regardless of how massive the parent database structure grows [[04:18](http://www.youtube.com/watch?v=FG5Varj1Ows&t=258)].
* **The Catch:** Op-based CRDTs mandate an underlying **causally consistent, reliable broadcasting network** [[05:46](http://www.youtube.com/watch?v=FG5Varj1Ows&t=346)]. Because mutations are chained sequentially, dropping or duplicating packets wrecks historical data integrity.
* *Example:* If an application triggers an `Add(Ham)` operation followed closely by a `Remove(Ham)` operation, and a regional partition drops the initial `Add` packet, a distant follower will attempt a `Remove` execution on an empty set, resulting in an unresolvable error state [[04:55](http://www.youtube.com/watch?v=FG5Varj1Ows&t=295)]. Operations are **not inherently idempotent**; executing an identical `Add(Ham)` command 5 times adds 5 distinct entities [[06:11](http://www.youtube.com/watch?v=FG5Varj1Ows&t=371)].



### B. State-Based CRDTs

Nodes continually transmit their **entire local dataset or vector map** across the network to peers [[06:36](http://www.youtube.com/watch?v=FG5Varj1Ows&t=396)].

* **The Merge Protocol:** Upon receiving an incoming foreign vector, the target server skips manual operational updates and feeds the foreign payload alongside its local data directly into a rigorous `merge()` function [[07:07](http://www.youtube.com/watch?v=FG5Varj1Ows&t=427)].
* **Mathematical Constraints:** For state convergence to be completely bulletproof across erratic networks, the underlying `merge()` mathematical engine must strictly satisfy three properties [[07:31](http://www.youtube.com/watch?v=FG5Varj1Ows&t=451)]:
1. **Commutative ($A \cup B = B \cup A$):** The ultimate system resolution must be identical regardless of whether Node 1 updates Node 2 or Node 2 updates Node 1 [[07:37](http://www.youtube.com/watch?v=FG5Varj1Ows&t=457)].
2. **Associative ($(A \cup B) \cup C = A \cup (B \cup C)$):** The literal arrival sequence of the state payloads across nodes has no bearing on final calculation accuracy [[07:49](http://www.youtube.com/watch?v=FG5Varj1Ows&t=469)].
3. **Idempotent ($A \cup A = A$):** Processing a single matching vector multiple times over does not alter or inflate data metrics [[07:55](http://www.youtube.com/watch?v=FG5Varj1Ows&t=475)].



### Networking: The Gossip Protocol

Because state-based CRDTs are resilient to ordering and duplication, they integrate perfectly with peer-to-peer **Gossip Protocols** [[08:45](http://www.youtube.com/watch?v=FG5Varj1Ows&t=525)]:

1. A local client mutation updates a single server node [[09:01](http://www.youtube.com/watch?v=FG5Varj1Ows&t=541)].
2. That node picks two random companion nodes across the network cluster and replicates its full vector payload to them [[09:01](http://www.youtube.com/watch?v=FG5Varj1Ows&t=541)].
3. In the subsequent time interval, those newly updated nodes pick two more random servers to infect with the state payload [[09:16](http://www.youtube.com/watch?v=FG5Varj1Ows&t=556)].
4. Even if individual nodes receive duplicate data structures through different routes, the **idempotent merge function** suppresses errors [[09:29](http://www.youtube.com/watch?v=FG5Varj1Ows&t=569)]. The payload cascades exponentially through the cluster until every single node reaches parity [[10:18](http://www.youtube.com/watch?v=FG5Varj1Ows&t=618)].

* *Limitation:* Payload payloads scale linearly with cluster sizes ($O(N)$ space); if data blocks are immense, this incurs severe network overhead and propagation lag [[10:33](http://www.youtube.com/watch?v=FG5Varj1Ows&t=633)].

---

## 3. Core CRDT Data Structures

### A. Distributed Counter (PN-Counter)

* **Grow-Only Counter (G-Counter):** Tracks increments across an array map tracking specific leader nodes [[11:34](http://www.youtube.com/watch?v=FG5Varj1Ows&t=694)]. The total count value equates to $\sum(\text{Vector Elements})$ [[11:52](http://www.youtube.com/watch?v=FG5Varj1Ows&t=712)].
* **Positive-Negative Counter (PN-Counter):** To support decrements, the database tracks **two separate internal vector logs**: an Increments Array and a Decrements Array [[12:14](http://www.youtube.com/watch?v=FG5Varj1Ows&t=734)].
* *Evaluation Formula:* To calculate the absolute value on demand, the server sums the elements of the increment vector and subtracts the sum of the decrement vector [[12:30](http://www.youtube.com/watch?v=FG5Varj1Ows&t=750)].

$$\text{Final State} = \sum(\text{Increments Vector}) - \sum(\text{Decrements Vector})$$





### B. Two-Phase Set (2P-Set)

* **The Architecture:** The database isolates mutations into two interior independent tables: an **Add Set** and a **Remove Set (Tombstone Log)** [[12:58](http://www.youtube.com/watch?v=FG5Varj1Ows&t=778)].
* **Evaluation Rule:** To render the current state of a set to a client, the database computes the mathematical difference between the internal groups ($\text{Add Set} - \text{Remove Set}$) [[13:11](http://www.youtube.com/watch?v=FG5Varj1Ows&t=791)].
* **The Flaw:** Once an element is recorded in the Remove Set, it can **never be re-added** to the active set [[13:39](http://www.youtube.com/watch?v=FG5Varj1Ows&t=819)]. If `Cheese` is inside the remove database, adding it to the add set again is ignored because the remove set signature acts as a permanent exclusion filter [[13:51](http://www.youtube.com/watch?v=FG5Varj1Ows&t=831)].

### C. Observed-Remove Set (OR-Set)

To allow items to be added, deleted, and safely re-added, systems drop 2P-Sets for **OR-Sets** [[14:02](http://www.youtube.com/watch?v=FG5Varj1Ows&t=842)].

* **Mechanics:** Every single `Add` instruction appends a unique, random metadata tracking identifier tag to the target value (e.g., inserting `(Ham, ID: 123)`) [[14:02](http://www.youtube.com/watch?v=FG5Varj1Ows&t=842)].
* **Re-Addition Proofing:** If a client deletes an item, the database copies that exact, specific tagged entity `(Ham, ID: 123)` into the removes collection [[14:26](http://www.youtube.com/watch?v=FG5Varj1Ows&t=866)]. If another parallel cluster node concurrently writes `(Ham, ID: 241)`, that specific insertion lacks a matching tombstone tag and remains actively visible to clients [[14:43](http://www.youtube.com/watch?v=FG5Varj1Ows&t=883)]. This tag-matching requirement completely removes the re-addition limit [[15:04](http://www.youtube.com/watch?v=FG5Varj1Ows&t=904)].

---

## 4. Sequence CRDTs (Teaser)

Managing conflict-free representations of ordered sequences (like lists or text strings) is incredibly complex because mutations rely on relational positions [[15:20](http://www.youtube.com/watch?v=FG5Varj1Ows&t=920)]. If multiple concurrent edits happen simultaneously without global coordinates, ordering becomes chaotic [[15:32](http://www.youtube.com/watch?v=FG5Varj1Ows&t=932)]. Specialized sequence CRDTs resolve this problem and serve as the core engine powering heavy collaborative editing workspaces like **Google Docs** and real-time remote IDEs [[15:53](http://www.youtube.com/watch?v=FG5Varj1Ows&t=953)].
