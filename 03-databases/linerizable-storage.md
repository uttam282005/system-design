# System Design Notes: Linearizable Databases

## 1. Defining Linearizable Storage

**Linearizability** (also known as strong consistency) is a correctness guarantee ensuring that a distributed storage system behaves as if there is only **one single copy of the data** in existence, and all operations on it are executed atomically [[01:33](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=93)].

* **The Core Requirement:** In a linearizable system, all writes must be strictly and universally ordered [[01:33](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=93)]. Once a write is successfully processed, any subsequent read anywhere in the cluster **must return that value or a newer one** [[01:45](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=105)].
* **The "Time-Travel" Invariant:** Reads are strictly prohibited from "going back in time" [[01:45](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=105)]. For instance, if Client 1 writes `X=5` and then Client 2 reads `X=5`, there must be absolutely zero possibility that Client 3 issues a subsequent read and receives an older value like `X=1` [[01:59](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=119)].

### Real-World Necessity:

Linearizability is vital for critical architectural operations such as:

1. **Distributed Locking:** Ensuring that exactly one client can hold a lock at any given moment [[00:45](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=45)]. If a network blip allows the storage state to go backward, a dead lock owner could be re-read as active, causing duplicate execution [[00:55](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=55), [12:07](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=727)].
2. **Database Leader Election:** Ensuring that the entire cluster unambiguously agrees on exactly one active leader node, preventing catastrophic split-brain scenarios [[01:13](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=73)].

---

## 2. Global Ordering Mechanisms

To achieve a system where data doesn't move backward, writes must be sequenced across machines [[02:19](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=139)].

### A. Replication Logs (Single-Leader Systems)

In a healthy single-leader setup, ordering is simple: the leader tracks all writes sequentially in a **Replication Log** (`Write #1`, `Write #2`, `Write #3`) and broadcasts it down to followers [[02:32](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=152)]. Every node in the cluster agrees on the ordering because the single leader dictates it [[02:46](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=166)].

### B. Version Vectors vs. Lamport Clocks (Multi-Leader/Leaderless)

When writes bypass a single leader and hit multiple distinct servers concurrently, physical timestamps fail to order operations due to clock skew [[02:55](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=175)]. Distributed systems turn to logical clocks [[03:17](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=197)].

* **Version Vectors ($O(N)$ Space):** These maps track structural concurrency by keeping operational counters for every node in the cluster [[03:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=202)]. However, their spatial footprint grows linearly with the number of nodes ($N$), causing network overhead in massive clusters [[04:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=262)].
* **Lamport Clocks ($O(1)$ Space):** Lamport clocks provide a highly compact total ordering of events across a cluster using only a single integer value per operation [[04:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=262), [07:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=442)].

---

## 3. Deep Dive: How Lamport Clocks Work

Every node (and every client) in the system maintains a localized logical counter, initialized to `0` [[05:16](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=316)].

### The Logical Rules:

1. **Before an event** (like a write or a network message broadcast), a node increments its local counter by `1`.
2. **When sending a message**, the sender attaches its updated counter value to the packet.
3. **When receiving a message**, the recipient compares its local counter with the counter stamped on the message. It sets its local counter to the **maximum** of the two values, and then adds `1` [[05:40](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=340)]:
$$\text{Counter}_{\text{local}} = \max(\text{Counter}_{\text{local}}, \text{Counter}_{\text{incoming}}) + 1$$



### Settle Ties (Total Ordering)

If Client A and Client B concurrently write to separate nodes, both operations might receive an identical logical counter value of `1` [[06:04](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=364), [08:13](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=493)]. To break the tie and establish a **Total Order**, the system appends the unique string identifier of the node itself to the counter: `(Counter, Node_ID)` [[07:39](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=459)].

* *Tie-Breaker Rule:* Sort strictly by the counter number first. If the counters match, resolve alphabetically/numerically by the `Node_ID` [[07:44](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=464)].
* *Example Sequence:* `(1, Node_A)` will always be ordered before `(1, Node_B)` [[08:13](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=493)]. This produces a clean, unbroken chronological timeline across every machine in the system [[08:25](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=505)].

---

## 4. The Flaw: Why Total Ordering $\neq$ Linearizable

A common mistake in systems design is assuming that because Lamport Clocks create a flawless total order of rights, the database is automatically linearizable [[08:31](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=511)]. **It is not.** Lamport clocks order writes *after the fact*, but they cannot prevent nodes from serving stale views in real-time over asynchronous networks [[08:41](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=521)].

### Failure Case 1: Multi-Leader Lag

* A write sets `X=5` on Node B with a Lamport stamp of `(1, Node_B)` [[09:13](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=553)].
* Simultaneously, Node A holds a stale value `X=1` [[09:13](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=553)].
* If a client queries Node A, it reads `X=1`. Then it queries Node B and reads `X=5` [[09:32](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=572), [09:40](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=580)]. If it queries Node A again before Node B's asynchronous replication packet arrives, it reads `X=1` once more [[09:45](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=585)].
* The system's reads have gone backward in time, violating linearizability [[09:50](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=590)].

### Failure Case 2: Single-Leader Crash (Failover Window)

Even single-leader databases fail linearizability under specific fault scenarios if replication is asynchronous [[10:23](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=623)]:

1. A client writes `X=5`, then writes `X=10` to the Leader [[10:34](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=634)].
2. The client executes a read from the Leader and safely receives `X=10` [[11:02](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=662)].
3. The Leader abruptly crashes *before* the log record `X=10` can be asynchronously copied to the Follower node [[11:13](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=673)].
4. The system triggers a failover and promotes the Follower to be the new Leader.
5. The client issues a subsequent read to the new Leader and receives `X=5` [[11:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=682)].

The client observed the system step back from `10` to `5` [[11:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=682)]. Because an established state disappeared upon a node failure, it is not a fault-tolerant linearizable system [[11:27](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=687)].

---

## 5. Summary Architectural Reference

| Feature | Totally Ordered Storage (Lamport Clocks) | Linearizable Storage (Strong Consistency) |
| --- | --- | --- |
| **Mechanics** | Assigns deterministic identifiers to rank events retroactively [[07:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=442)]. | Guarantees immediate real-time synchronization across reads and writes [[01:33](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=93)]. |
| **Spatial Cost** | Highly optimal ($O(1)$ constant space overhead) [[04:22](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=262)]. | Heavy network and coordination overhead across nodes. |
| **Time-Travel Protection** | **None.** Clients can easily view old or mismatched data states due to replication lag [[09:50](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=590)]. | **Absolute.** Reads are mathematically blocked from seeing stale data states [[01:45](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=105)]. |
| **Requirement** | Simple logical counter bumping rules [[05:40](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=340)]. | Requires **Total Order Broadcast** via **Distributed Consensus** (e.g., Paxos, Raft) [[11:47](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=707), [12:25](http://www.youtube.com/watch?v=C_XLEeWUq3M&t=745)]. |
