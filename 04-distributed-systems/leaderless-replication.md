# System Design Notes: Leaderless Replication Introduction

## 1. Core Paradigm shift

Leaderless replication (also known as active-all or Dynamo-style replication) completely abandons the concept of a single authoritative leader node [[00:39](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=39)].

* **The Mechanism:** Clients send modifications (writes) concurrently to a group of coordinate database nodes [[00:45](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=45)]. Similarly, when reading data, clients fetch documents from multiple separate nodes in parallel [[00:55](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=55)].
* **Real-World Examples:** This strategy is heavily popularized by open-source distributed databases like **Apache Cassandra** and **Riak** [[01:15](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=75)].
* *Interview Clarification:* While this pattern was originally outlined in Amazon's foundational "Dynamo paper," Amazon's modern commercial **DynamoDB** cloud service does *not* use leaderless replication; it operates on a single-leader framework per partition [[01:28](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=88)].

---

## 2. Resolving Conflicting States

Because writes flow directly to any available server indiscriminately, nodes regularly fall out of sync or hold old information due to minor network delays or system crashes [[02:12](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=132)].

To ensure clients consistently fetch the absolute truth, systems apply two vital mechanisms: **Read Repair** and **Anti-Entropy** [[03:36](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=216), [03:52](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=232)].

### A. Version Tracking & Read Repair

When writing data, the storage layer stamps an incremental version or token tracking number to the data object [[02:17](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=137)].

1. **Parallel Read Fetch:** A client reads a key (`Jordan`) from three independent nodes simultaneously [[02:35](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=155)].
2. **Evaluating States:** * Node 1 returns: `[Version 21, Value: cute]`
* Node 2 returns: `[Version 21, Value: cute]`
* Node 3 returns: `[Version 20, Value: ugly]` [[02:35](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=155)]


3. **Client Determination:** Since version `21 > 20`, the client knows with certainty that `cute` is the accurate current global value [[02:48](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=168)].
4. **Read Repair Activation:** Recognizing that Node 3 holds an outdated block of data, the client or coordinator triggers an asynchronous background write back to Node 3 detailing the exact values of version 21 [[03:02](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=182)]. Node 3 applies the update, healing the database cluster on demand [[03:23](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=203)].

---

## 3. Anti-Entropy Protocols & Merkle Trees

While read repair heals stale items when they are explicitly touched by application reads, background nodes still need a structural strategy to constantly reconcile differences across the entire data set [[03:42](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=222)]. This background healing background script is called **Anti-Entropy** [[03:52](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=232)].

### The Scale Problem

In master-slave systems, a replica just looks at a single sequential replication log stream [[04:26](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=266)]. In leaderless designs, every node accepts distinct mutations independently [[04:00](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=240)]. Safely scanning millions of rows across two multi-terabyte tables linearly to identify localized differences creates a catastrophic $O(N)$ scanning bottleneck and massive cross-network overhead [[05:14](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=314)].

### The Solution: Merkle Trees (Cryptographic Tree Structures)

To verify data mismatches in logarithmic time ($O(\log N)$), leaderless databases construct internal **Merkle Trees** (binary cryptographic hash trees) over data tables [[05:42](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=342), [09:27](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=567)].

#### How a Merkle Tree is Built:

1. **Leaf Level:** The database takes a cryptographic hash of every single individual data row in a specific table block [[06:23](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=383)].
2. **Internal Node Level:** The database clusters pairs of leaf hashes, strings their values together, and hashes the combined summation string [[06:34](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=394)].
* *Example:* $\text{Hash}(\text{Hash}(\text{Row A}) + \text{Hash}(\text{Row B}))$ [[06:41](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=401)].


3. **Root Node:** This matching pair-hashing steps up sequentially through the binary tree branches until a single **Root Hash** is produced [[06:54](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=414)]. If two database tables are 100% identical down to the bit, their final Root Hashes will match perfectly.

---

## 4. Reconciling Data via Merkle Trees

When two nodes need to run anti-entropy background synchronization, they exchange only their high-level Merkle tree structures over the network and execute a Breadth-First Search (BFS) [[07:52](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=472)]:

1. **Root Check:** Node 1 reads Node 2's Root Hash. If Root `305 != 642`, the nodes know a data variance exists somewhere below [[08:00](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=480)].
2. **Branch Check:** They compare the left and right child hashes [[08:09](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=489)].
* Right Branch: Hash `463 == 463`. The entire right subset of rows is verified as completely matching. The anti-entropy engine completely skips evaluating this half of the database [[08:21](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=501)].
* Left Branch: Hash `119 != 505`. The mismatch is isolated to the left side [[08:14](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=494)].


3. **Leaf Resolution:** The algorithm descends down the left branch. It checks the final leaves and identifies that while row `A` matches (`746 == 746`), row `B` does not match (`821 != 314`) because Node 1 holds `B=6` and Node 2 holds `B=9` [[08:47](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=527)].
4. **Network Delta Sync:** Instead of shipping the entire table block, Node 1 sends *only* the single specific corrected row payload for row `B` across the network [[09:07](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=547)]. Node 2 absorbs the edit and adjusts its tree branch [[09:13](http://www.youtube.com/watch?v=Jy4Cm2WEZVg&t=553)].

---

## 5. Technical Trade-Off Reference

| Metric | Single-Leader Replication | Leaderless Replication |
| --- | --- | --- |
| **Write Bottleneck** | High (All writes funnel through one machine). | Low (Writes scale across all nodes simultaneously). |
| **Network Payload for Sync** | Continuous, orderly streaming of single updates via standard Logs. | Batched anti-entropy exchanges using highly efficient Merkle trees ($O(\log N)$ complexity). |
| **Read Consistency Strategy** | Reads from a single node provide predictable, immediate state timelines. | Reads must query multiple nodes simultaneously to dynamically isolate stale rows via version logic. |
| **System Complexity** | Simpler handling of failures through standard master-failovers. | Complex; requires background processes to actively manage inconsistencies. |
