## 1. The Context of Distributed Consensus

Distributed consensus is the foundational primitive required to build **linearizable storage systems**. Linearizability provides an absolute guarantee that every process in a cluster observes a single, unified, forward-progressing timeline of data states.

Building linearizable utilities—such as **distributed locking frameworks** or **service discovery mechanisms**—requires establishing a consistent ordering of operations across a network.

### Why Do We Use a Log-Structured State Machine?

To achieve sequential ordering, consensus protocols compile application mutations into an ordered append-only stream called a **Distributed Log**. Because a log enforces strict chronological sequence, it guarantees a linearizable execution timeline.

The **Raft Consensus Algorithm** is an industry-standard framework designed to manage and replicate this distributed log across an independent pool of commodity machine nodes. Raft relies on a strongly asymmetric design, meaning a single node is explicitly chosen to act as the authoritative manager of the global state machine.

---

## 2. Raft Node States & The Core Lifecycle

At any moment, an individual machine node within a Raft cluster operates in precisely one of three structural roles:

```
    ┌───────────────┐
    │   Follower    │
    └───────┬───────┘
            │ (Election Timeout Triggers)
            ▼
    ┌───────────────┐  (Wins Quorum Majority)  ┌───────────────┐
    │   Candidate   │ ───────────────────────► │    Leader     │
    └───────────────┘                          └───────────────┘

```

1. **Leader:** The master node that handles incoming client write mutations, manages log entry positions, and actively orchestrates replication across the rest of the cluster.
2. **Follower:** Completely passive workers. They do not initiate actions; they merely accept log writes and command parameters streamed asynchronously from the active leader.
3. **Candidate:** A temporary transitional state assumed by a follower node when it actively attempts to trigger an election to claim cluster leadership.

---

## 3. The Mechanics of Raft Leader Election

### Step 1: Heartbeats & Randomized Election Timeouts

Under normal operational health, the cluster leader continuously broadcasts lightweight network pings called **Heartbeats** to all follower nodes at a regular time interval. These heartbeats signal to the followers that the leader is alive and active.

If a follower stops receiving heartbeats (e.g., due to a leader crash or network partition), it must initiate an election. To prevent every follower node from noticing the leader's absence and declaring an election at the exact same millisecond—which would flood the network and stall consensus—Raft employs **Randomized Election Timeouts**:

* Each follower node is independently assigned a randomized countdown timer (e.g., Node 1 waits 10 seconds, while Node 2 waits 5 seconds).
* Whichever node features the shortest randomized countdown window (Node 2) will timeout first. It transitions its state to a **Candidate** and triggers the global election cycle.

### Step 2: The Concept of Terms (Epoch Numbers)

Time in Raft is broken down into continuous, monotonically increasing numerical blocks called **Terms** (or Epoch Numbers). Terms act as a logical clock system to identify legacy operations.

When a follower transitions to a Candidate, it increments the cluster's last known term number by 1 (e.g., progressing from Term 28 to Term 29) and votes for itself. It then broadcasts a request-for-vote RPC message to the remaining cluster nodes, identifying itself as the candidate seeking leadership for Term 29.

### Step 3: Evaluating Votes & The Postings Check

When a node receives a vote request from a Candidate, it evaluates the request using strict programmatic rules. There are four potential processing paths depending on the receiver's local state:

1. **Receiver is the Legacy Leader:** If the old leader receives a message from a candidate showcasing a higher term number (Term 29 > Term 28), it recognizes it has been disconnected or outdated. It instantly abdicates its power and demotes itself back to a passive **Follower** for Term 29.
2. **Receiver is a Lagging Follower:** If the follower's local term is lower than the one being proposed, it updates its local term counter to match the new Term 29.
3. **Receiver Discovers a Conflicting Active Leader:** If a node has already voted for or recognized a different leader for the current Term 29, it explicitly returns a **`NO` vote** to block the candidate.
4. **The Structural Log-Completeness Check:** To secure a positive **`YES` vote**, a Candidate must satisfy two distinct criteria:
* The candidate's term number must be strictly greater than the receiver’s local term counter.
* **The Log-Safety Guard:** The candidate’s log must be **at least as up-to-date as the receiver’s local log**. The node cross-references the index and term of the last entry in both logs. If the candidate's log is shorter or less complete than the follower's, the vote is rejected. This ensures that a node missing committed entries can never be elected leader.



---

## 4. Winning the Election & Safety Invariants

To successfully claim leadership, a Candidate must collect positive vote confirmations from a strict **Quorum Majority** of the total cluster nodes (e.g., collecting at least 2 out of 3 votes, or 3 out of 5).

Once won, the node transitions to a **Leader** and instantly resumes broadcasting heartbeats to assert control over the cluster.

Raft’s leader election mechanics enforce three critical safety invariants:

### 1. Unique Leader Guarantee (No Split-Brain)

Because electing a leader requires a strict mathematical quorum majority, it is physically impossible to elect two separate leaders during the identical term. Since any two quorums must overlap by at least one node, that overlapping node would be forced to vote twice in the same term, which the protocol strictly forbids.

### 2. Old Leader Fencing

Terms function as a high-security **Fencing Token**. If an old, partitioned leader attempts to issue commands to followers after a new election has resolved, its messages will contain a stale term number (e.g., Term 28). Follower nodes inspect the fencing token, identify it as outdated compared to their current counter (Term 29), and instantly reject the commands.

### 3. Leader Completeness (Backfilling)

Because a node cannot win an election unless its log is equal to or more complete than a majority of nodes, the newly elected leader is guaranteed to possess **every single entry that has been successfully committed in previous terms**.

Once active, the new leader acts as the ultimate source of truth, using its log to safely overwrite and **backfill** any lagging or stale follower nodes across the cluster. If a follower possesses a sporadic, uncommitted write that is more up-to-date than the leader's log, it implies that write never achieved a quorum majority in the first place, and the leader will safely purge it during synchronization.

---

## 5. Summary Matrix: Node State Constraints

| Node Role | Handles Client Mutations? | Initiates Cluster Elections? | Transmits System Heartbeats? | Key Log Boundary Invariant |
| --- | --- | --- | --- | --- |
| **Leader** | **Yes** (Accepts and sequences all writes). | No. | **Yes** (Continuously pings followers). | Must possess all previously committed global logs. |
| **Follower** | No (Rejects or redirects writes to leader). | No (Passive worker). | No. | Log index can temporarily lag behind the leader. |
| **Candidate** | No. | **Yes** (Increments term counters). | No. | Log must be at least as complete as a quorum majority to win. |
