## 1. Introduction to Two-Phase Locking (2PL)

In the previous lesson, we looked at achieving serializability through **Actual Serial Execution** (eliminating concurrency by running everything on a single thread). However, before modern fast CPUs made that viable, traditional relational databases relied on **Two-Phase Locking (2PL)** to allow concurrent transaction execution while still guaranteeing full serializability.

* **The Core Definition:** 2PL ensures that concurrent transactions execute in a way that is mathematically equivalent to running them one at a time (serializability).
* **Historical Context:** This is the blocking mechanism historically used by standard SQL relational databases when multi-threading on multi-core CPUs is required.

---

## 2. Lock Modes: Shared vs. Exclusive Access

In basic database isolation, a lock often means "exclusive access." 2PL introduces a more granular approach by categorizing locks into two distinct modes to maximize read concurrency while preventing write conflicts:

### A. Shared Lock (Reader Mode / `S-lock`)

* Multiple transactions can simultaneously hold a Shared Lock on the same row.
* **Rule:** If transaction A holds a Shared Lock on a row, transaction B can also instantly acquire a Shared Lock on it to read it. No one is blocked.

### B. Exclusive Lock (Writer Mode / `X-lock`)

* Only a single transaction can hold an Exclusive Lock on a row.
* **Rule:** If a transaction wants to write to or modify a row, it must acquire an Exclusive Lock. If any other transaction is currently reading that row (holding a Shared Lock), the writer must stall and wait for **all** readers to finish and release their locks.
* Conversely, if an Exclusive Lock is held on a row, all other reads and writes to that row are blocked.

### Preventing Race Conditions

This locking matrix natively solves the classic **Read-Modify-Write cycle** race condition. Because a transaction holds its reader lock while computing its next step, no other transaction can sneak in, grab an exclusive lock, and change the data under the hood. The predicate (the data read at the start) remains perfectly valid.

---

## 3. The Two Phases of 2PL

The name "Two-Phase Locking" comes from the strict rule governing how locks are acquired and released during a single transaction's lifecycle:

```
[ Growing Phase: Acquiring Locks ] ---> ( Peak Lock State ) ---> [ Shrinking Phase: Releasing Locks ]

```

1. **The Growing Phase:** The transaction can acquire new locks or upgrade a Shared Lock to an Exclusive Lock. It is **not allowed to release any locks** during this phase.
2. **The Shrinking Phase:** The transaction begins releasing locks. Once the very first lock is released, the transaction enters the shrinking phase and **can never acquire or upgrade another lock** for the rest of its duration.

---

## 4. The Critical Bottleneck: Deadlocks

The most important operational drawback of 2PL is the frequency of **deadlocks**. Deadlocks can occur even if transactions attempt to access data resources in the exact same logical order.

### The Shared-to-Exclusive Upgrade Deadlock Example

Imagine two users, Jordan and Snoop Dogg, executing identical transactions simultaneously to copy items into their own shopping carts:

1. **Jordan's Transaction** acquires a **Shared Lock** on Jordan's row and a **Shared Lock** on Snoop's row to read the data.
2. **Snoop's Transaction** simultaneously acquires a **Shared Lock** on Snoop's row and a **Shared Lock** on Jordan's row.
3. Both transactions now try to upgrade their respective locks to **Exclusive Locks** to write the updates.
4. **The Standoff:** Jordan's transaction cannot get an Exclusive Lock on Jordan's row because Snoop holds a Shared Lock on it. Snoop's transaction cannot get an Exclusive Lock on Snoop's row because Jordan holds a Shared Lock on it.

Both threads are permanently frozen waiting for the other to release their shared lock.

### Mitigation Cost

Because deadlocks are a structural reality of 2PL, the database must continuously run background processes to detect deadlock cycles. When a deadlock is found, the database must forcibly **abort and roll back** one of the transactions, releasing its locks so the other can complete, and then retry the aborted transaction from scratch. This constant cycle of locking overhead, detecting deadlocks, and aborting transactions makes 2PL inherently slow at high concurrency.

---

## 5. Solving "Phantoms" (Locking Data That Doesn't Exist Yet)

Standard row-level locks only protect rows that *already exist* in a table. They fail to prevent **Phantoms**—a race condition where a concurrent transaction inserts *new* rows that alter the result of a prior search query.

> **The Phantom Example:** A fitness class has an enrollment limit of 3 people, and 2 people are already registered. Jordan and his friend simultaneously check the count. Both read a count of 2, so both proceed to insert a new row to join. Since neither row existed beforehand, row locks couldn't stop them. The database ends up over-enrolled with 4 people.

To achieve true serializability, databases use two advanced forms of locking to stop phantoms:

### Approach A: Predicate Locking

Instead of locking specific rows, the database locks a *condition* or *predicate*.

* **Mechanism:** If you query `WHERE class_name = 'flexibility' AND class_time = '6PM'`, a Predicate Lock freezes that entire conditional matching space. Any concurrent attempt by another transaction to insert a row matching that exact condition will be blocked.
* **The Downside:** Predicate locks are heavily resource-intensive. If there isn't a perfect database index for the exact columns in the predicate, checking whether a new insert violates an active predicate lock requires a slow, expensive scan of all active locks.

### Approach B: Index-Range Locking (Next-Key Locking)

Because pure predicate locks are too slow, almost all real-world databases use **Index-Range Locking** as a faster approximation.

* **Mechanism:** Rather than locking a complex phrase, the database leverages an existing index (e.g., an index on `class_name`). It will look up the value `'flexibility'` in the index and **lock the entire range slot** around that index value.
* **The Trade-off:** Index-range locking effectively locks a *superset* of data. It might accidentally lock out an insertion for a 7 PM flexibility class even though you only cared about the 6 PM class. While it is drastically faster to evaluate than a predicate lock (running in $O(\log N)$ time), it reduces throughput by blocking completely safe transactions that just happen to fall within the locked index bracket.
