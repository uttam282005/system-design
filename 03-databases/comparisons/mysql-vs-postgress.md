## 1. Core Mechanics of SQL Databases

Relational (SQL) databases are built to manage normalized data structures. In a normalized schema, the system maintains a single, authoritative reference for each piece of data, avoiding duplication. This ensures absolute consistency across complex table structures, but it heavily relies on **ACID-compliant transactions** to eliminate race conditions and data conflicts.

### Shared Features of Relational Engines

* **B-Tree Indexing Architecture:** Unlike many write-optimized NoSQL systems that utilize Log-Structured Merge-trees (LSM-Trees), traditional relational engines default to **B-Trees** for storage indexing.
* In an LSM-Tree architecture, a query must check an in-memory Memtable before checking multiple Sorted String Tables (SSTables) on disk.
* In a B-Tree model, the tree structures are navigated directly on disk, allowing read operations to instantly jump to a specific location. This makes B-Trees optimized for fast, predictable reads.


* **Single-Leader Replication:** While open-source extensions can modify topology, SQL databases default to a single-leader replication layout. All write operations route strictly through a single primary leader node to establish a global chronological order. This avoids the concurrent write conflicts (e.g., split-brain or stale updates) common in multi-leader or leaderless setups.
* **Tunable Isolation Levels:** Relational databases allow developers to configure transactional boundaries (such as Repeatable Read or Serializable) to dictate how concurrent changes are isolated from one another.

---

## 2. Concurrency Control: MySQL vs. PostgreSQL

The most distinct structural difference between MySQL and PostgreSQL lies in how they implement the highest level of transactional boundary: **Serializable Isolation**.

### MySQL: Two-Phase Locking (2PL)

MySQL (traditionally leveraging the InnoDB storage engine) achieves serializability through a pessimistic framework known as **Two-Phase Locking (2PL)**.

* **Shared vs. Exclusive Locks:** When a transaction reads a row, it must acquire a *Shared (Read) Lock*. Multiple transactions can hold shared locks on the same row simultaneously. However, if a transaction wants to modify a row, it must acquire an *Exclusive (Write) Lock*. This blocks all other incoming reads or writes until the lock is released.
* **The Performance Penalty:** Because transactions must wait for locks to clear, 2PL introduces significant system latency. Furthermore, if two concurrent transactions attempt to claim overlapping locks in reverse sequences, they trigger a **Deadlock**, forcing the engine to abort and clean up the process.

### PostgreSQL: Serializable Snapshot Isolation (SSI)

PostgreSQL handles serializable isolation using an optimistic framework called **Serializable Snapshot Isolation (SSI)**.

* **Optimistic Execution:** SSI assumes that data collisions are rare. Instead of blocking operations with locks, a transaction reads data from a localized **Consistent Snapshot** of the database. It modifies its data independently within this isolated sandbox.
* **Commit-Time Validation:** The moment the transaction attempts to commit its changes, the engine cross-references the snapshot. If a separate concurrent transaction has modified any of the underlying data since the snapshot was captured, a collision is flagged. The engine rejects the transaction and forces a **Rollback**.

---

## 3. Concurrency Strategy Matrix

| Architectural Axis | MySQL (Two-Phase Locking) | PostgreSQL (Serializable Snapshot Isolation) |
| --- | --- | --- |
| **Concurrency Class** | **Pessimistic** Control | **Optimistic** Control |
| **Primary Mechanism** | Blocks rows using Shared & Exclusive locks. | Isolates data via localized snapshots. |
| **Collision Handling** | Blocks execution thread; waits for lock release. | Aborts the transaction and issues a Rollback. |
| **Primary Failure Risk** | Deadlock conditions requiring manual resolution. | High write-collision rates under heavy contention. |

---

## 4. System Design Selection Heuristic

### When to choose MySQL (Pessimistic 2PL)

Pessimistic locking is optimal for **High-Contention Use Cases** where multiple concurrent threads frequently modify the exact same keys (e.g., a high-volume counter, inventory decrements, or a global ticketing system).

Under intense data contention, PostgreSQL's optimistic model would trigger constant validation collisions, forcing the system to roll back and re-run up to 90% of its writes. MySQL's lock-based approach forces transactions to line up sequentially. While this introduces queue latency, it eliminates the resource waste of constant transactional rollbacks.

### When to choose PostgreSQL (Optimistic SSI)

Optimistic snapshot isolation shines in **Low-Contention Use Cases** where transactions process wide-ranging analytical queries across separate records (e.g., standard social feeds, user profile updates, or multi-tenant SaaS dashboards).

Because collisions are rare in these workflows, PostgreSQL avoids the CPU and memory overhead of managing millions of active row-level locks. Transactions read and write concurrently without blocking each other, resulting in significantly higher system throughput than a pessimistic database.
