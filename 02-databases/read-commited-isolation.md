**Read Committed Isolation in Databases**

**Background: Concurrency and Race Conditions**
*   Databases are multi-threaded systems where multiple reader and writer threads execute concurrently on the same computer. 
*   Because the exact order of these executions is unpredictable, it can introduce race conditions that threaten the correctness and legitimacy of the database's results. 

**Key Terminology: Committing**
*   A **commit** means that a write operation is officially confirmed in the database. 
*   If a sequence involves multiple writes, those writes are only considered committed once all of them are successful. 

**Race Condition 1: Dirty Writes**
*   **Definition:** A dirty write occurs when a database writes over uncommitted values.
*   **Example:** Two threads attempt to update the same two rows in different tables (e.g., a "purchaser" row and a "delivery address" row) at the same time. The writes might interleave, allowing one thread to overwrite the purchaser value while the other thread overwrites the delivery address, leading to inconsistent results that break database invariants.
*   **Solution:** Dirty writes can be fixed using **row-level locking**. By grabbing a lock on a specific row, a thread prevents any other thread from modifying that row until the lock is released. 
*   **Deadlock Mitigation:** To prevent deadlocks—where two threads hold locks and infinitely wait for each other's resources—databases can mandate that rows are grabbed in a specific order, or they can utilize deadlock detection to force a transaction to give up its locks.

**Race Condition 2: Dirty Reads**
*   **Definition:** A dirty read occurs when a thread reads uncommitted data.
*   **Example:** A transaction deducts $10 from a bank account, and a separate thread immediately reads the new, lower balance. If the original transaction fails to send the money to the recipient and cannot commit, the reader thread has read an invalid, uncommitted state.
*   **Solution:** While row-level locking could technically prevent dirty reads, it is avoided because waiting on locks is slow and a single write would heavily bottleneck much faster read operations. The better solution is to **store the old value** until the commit completes. The database continues to provide the old value to readers until the new write is finalized, at which point the pointer is switched to the new value. This reduces overhead and speeds up the database by avoiding locks.

**Conclusion**
*   **Read Committed Isolation** is defined as a database isolation level that successfully protects against both dirty reads and dirty writes. 
*   While read committed isolation prevents these two specific issues, there are other, more complex race conditions in databases that require even more advanced solutions.
