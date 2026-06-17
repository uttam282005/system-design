## 1. Introduction to Raft Writes

In the Raft consensus algorithm, all state mutations must route strictly through the single elected cluster **Leader**. Followers are completely passive; they cannot accept direct writes and must redirect client traffic to the leader.

The primary engineering goal of a Raft write operation is to safely replicate data across an independent pool of nodes while constructing an identically ordered **Distributed Log**. However, because followers can lag behind due to network delays or crashes, the leader cannot assume every node shares the same history. A write operation must actively repair log gaps via **backfilling**.

---

## 2. The Log-Matching Invariant

Raft enforces strict data consistency by maintaining a core structural rule known as the **Log-Matching Invariant**.

### The Rule

If two independent logs contain an entry at a given index with an identical **Term Number**, then those two logs are mathematically guaranteed to be **completely identical across all indexes preceding that point**.

### Core Terminology

* **The Prefix:** The historical segment of the log to the left of a target index where both the leader and follower match precisely.
* **The Suffix:** The newer segment of the log to the right where a lagging follower's entries differ from or are missing compared to the leader's data.

Because writes backfill sequentially, and because only a single leader can be elected per term, matching terms at a given index prove that the identical leader sequenced both entries—and consequently, that leader already unified the history prior to that point.

---

## 3. The Write and Backfill Lifecycle

When a client issues a write to the leader, the engine triggers an iterative lookup loop to find the exact point where a follower's log diverges. This loop relies on a **Prefix-Suffix validation optimization** to minimize network usage.

### Step 1: The Initial Optimistic Proposal

The leader assigns the new client operation a term and index, appending it to its local log. It then tries to transmit the new entry to a follower, specifying the immediately preceding entry as the required **Prefix** (e.g., asserting that the follower must possess entry `C` from Term `21` at Index `2` to accept the new entry `D` for Term `22`).

### Step 2: Handling Rejections (Stepping Back)

If a follower is lagging, it won't possess entry `C` at Index `2`. It will reject the write request and signal a failure back to the leader.

Instead of transmitting its entire multi-gigabyte log over the network to fix the gap, the leader steps back by exactly one index position. It lowers its prefix threshold and tries again, this time asking if the follower matches entry `B` from Term `20` at Index `1`.

### Step 3: Resolving Alignment & The Suffix Stream

The leader continues stepping back index-by-index until it finds a point where the follower confirms a prefix match.

* Once a match is established, the follower deletes any conflicting or uncommitted data to the right of that index.
* The leader then streams *only* the missing **Suffix** payload over the network.
* This targeted backfilling approach avoids transferring the entire log, significantly saving cluster network bandwidth.

---

## 4. The Commit Pathway (Two-Phase Dynamic)

Just because a follower accepts a suffix stream onto its local disk does not mean the write is finalized. Raft processes updates using a two-phase consensus model:

1. **Phase 1: Local Logging:** Followers write the incoming suffix stream into a temporary, uncommitted log buffer and return a success acknowledgment to the leader.
2. **Phase 2: The Quorum Commitment:** The leader counts the incoming confirmations. The moment the entry is successfully written to a **Quorum Majority** of nodes, the leader permanently commits the transaction to its local state machine.
3. **Global Broadcast:** On subsequent heartbeat loops, the leader informs the rest of the cluster that the entry is officially committed. Followers then safely move the entry from their temporary buffers into their permanent local state machines.

> **The Quorum Safety Invariant:** Requiring a strict majority confirmation to commit an entry guarantees that the write will survive future failures. Since electing a new leader requires a vote from a majority of nodes, any future quorum of voters must overlap with at least one node that possesses this committed write. That node will block any candidate missing the entry, ensuring the write is preserved and backfilled across the cluster.

---

## 5. Architectural Trade-offs and Limitations

### The Latency Bottleneck

While Raft provides strong fault tolerance and absolute linearizability, it is inherently slow. Every read and write transaction is bottlenecked by the CPU processing limits of a single leader node and requires blocking network round-trips to achieve quorum consensus.

Consequently, Raft should not be deployed as a general-purpose application database for high-volume traffic (such as user timelines or chat streams). It is best used to build specialized cluster management components—like distributed locks, configuration providers, or coordination services (e.g., etcd, ZooKeeper)—where data consistency is paramount.

### Raft vs. Two-Phase Commit (2PC)

A common system design interview mistake is assuming Raft replaces the **Two-Phase Commit (2PC)** protocol. They solve entirely separate distributed systems problems:

* **Raft:** Manages **homogeneous replication**. Its goal is to maintain an identical copy of a single distributed log across a pool of redundant servers. It requires only a **Quorum Majority** to progress.
* **Two-Phase Commit (2PC):** Manages **heterogeneous distributed transactions**. It coordinates completely different write mutations across separate sharded partitions (e.g., deducting money from a user balance partition on Server A while creating an order log on Server B). 2PC requires an absolute **Unanimous Agreement (100% Yes)** from all participating nodes; if a single server is offline, the entire transaction rolls back.

*Note: Production architectures frequently combine both systems, utilizing individual Raft clusters to handle high-availability replication within shards, while orchestrating cross-shard transactions using a Two-Phase Commit layer.*
