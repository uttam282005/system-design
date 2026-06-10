# Systems Design Notes: Serializable Snapshot Isolation (SSI)

## 1. Introduction: Pessimistic vs. Optimistic Concurrency Control

To understand Serializable Snapshot Isolation (SSI), we have to look at the fundamental philosophical difference between how databases manage concurrent data access:

* **Pessimistic Concurrency Control (e.g., Two-Phase Locking / 2PL):** Assumes the worst. It assumes transactions *will* conflict, so it forces every transaction to aggressively grab reads and writes locks upfront. This slows down performance because transactions that never would have touched the same data are forced to wait in lines.
* **Optimistic Concurrency Control (OCC - e.g., SSI):** Assumes the best. It assumes that most transactions are independent and won't step on each other's toes. Instead of blocking with locks, it lets transactions run completely parallel. **It only checks for conflicts right before committing.** If a conflict is detected, it aborts the offending transaction and restarts it.

SSI is a modern, highly performant optimistic concurrency control design utilized by advanced databases (like PostgreSQL since version 9.1).

---

## 2. What is a Database Snapshot?

SSI builds directly on top of **Snapshot Isolation**.

* **Definition:** A snapshot is a completely consistent, frozen state of the database at a specific point in time (e.g., right after a specific transaction ID, like `Tx18`, commits).
* When a transaction starts under snapshot isolation, it only reads data from that specific point in time. Any changes made by other concurrent transactions are invisible to it, preventing dirty reads.

---

## 3. How SSI Detects Conflicts Without Locking

Because SSI allows multiple transactions to read and write to their own snapshots without holding physical locks, it has to actively watch for **premise/predicate violations**. If Transaction A modifies data that Transaction B *read as its premise to perform an action*, a conflict occurs.

SSI tracks this using two core execution patterns:

### Case 1: Reading Data with an Uncommitted, In-Flight Write

This happens when Transaction B reads a row while Transaction A has an active, uncommitted write pending on that same row.

```
Timeline:
[T19: Tx_A sets Status = False (Uncommitted)] 
                                      ↓
                     [T20: Tx_B reads Status (Sees True, but notices Tx_A's pending write)]
                                      ↓
[T21: Tx_A Commits]
                                      ↓
                     [Tx_B attempts to Commit] ---> SSI Detects Violation! -> Aborts Tx_B

```

* **The Setup:** Transaction A (`Tx_A`) alters a row's status from `True` to `False` but hasn't committed yet. Transaction B (`Tx_B`) reads the row from its snapshot, so it still sees `True`.
* **The Detection:** SSI doesn't block `Tx_B`. Instead, `Tx_B` explicitly notes: *"I am reading a value that has an active, uncommitted write against it."*
* **The Resolution:** When `Tx_B` eventually tries to commit, it checks if `Tx_A` successfully committed its write. Because `Tx_A` did commit, the premise `Tx_B` relied on (`True`) is now invalid. SSI forces `Tx_B` to **abort and restart**.

### Case 2: A Write Occurs *After* a Read (Tracking Intersecting Transactions)

This occurs when multiple transactions read a valid row, and a completely separate transaction comes in later to modify that row before the readers have committed.

```
Timeline:
[T19, T20, T21: Tx_1, Tx_2, Tx_3 read Row_X (Value = True)]
                                      ↓
                     [T22: Tx_4 changes Row_X to False and Commits]
                                      ↓
[Tx_1, Tx_2, Tx_3 attempt to Commit] ---> SSI Detects Violation! -> Aborts all three

```

* **The Setup:** Three concurrent transactions (`Tx_1`, `Tx_2`, `Tx_3`) read `Row_X` while it is `True`. They are preparing to write dependencies based on this fact.
* **The Detection:** The database row itself acts as a tracker. It maintains a list of all active, in-flight transactions currently reading it.
* **The Resolution:** A new transaction (`Tx_4`) arrives and explicitly changes `Row_X` to `False` and commits. The database looks at the row's tracker list, sees that `Tx_1`, `Tx_2`, and `Tx_3` relied on the old `True` value, and instantly tags them as poisoned. When those three try to commit, SSI intercepts them, **aborts them all**, and forces a restart.

---

## 4. Architectural Summary & When to Use SSI

Optimistic concurrency control like SSI dramatically improves performance under the right conditions, but it is not a silver bullet.

| Concurrency Strategy | Best Workload Condition | Performance Characteristic |
| --- | --- | --- |
| **Serializable Snapshot Isolation (SSI)** | **Disjoint Workloads:** Most transactions touch different data (e.g., users posting independently on their own social media profiles). | **High Throughput:** Zero lock contention overhead. Fast execution paths. |
| **Two-Phase Locking (2PL)** | **High Contention Workloads:** Many transactions fighting over the exact same rows (e.g., a highly distributed global counter or a ticket booking hot-seat). | **Stable but Slower:** Prevents a catastrophic chain reaction of transaction aborts and infinite loops of retries. |

> **The SSI Failure State:** If you use SSI on a high-contention counter where every single transaction reads a number and increments it, every transaction will constantly poison and abort every other transaction. The system will waste 100% of its CPU power processing transactions only to throw the work away at the commit line. For high-contention, use pessimistic locking (2PL). For low-contention/disjoint writes, use SSI.
