**Snapshot Isolation and Read Skew in Databases**

**The Race Condition: Read Skew**
*   **Definition:** Read skew (also related to repeatable read) is a database race condition where an invariant appears to be broken during a long-running read operation, such as an analytical query, because a write operation executes in the middle of it. 
*   **Example:** Imagine a table of bank accounts where the database invariant dictates that all balances combined must exactly add up to $1 million. 
*   If a long query begins reading the rows one by one, and halfway through, a separate transaction transfers money from an unread account to an already-read account, the final sum of the read will be incorrect. 
*   Even though the overall database remains in a valid state and the query only reads committed data, the read operation gathers an inconsistent view because it misses the transferred funds that were moved behind its progress. 

**Structuring Transactions: The Write-Ahead Log**
*   To solve this inconsistency, databases can utilize the write-ahead log (WAL), which is a sequential list on the disk. 
*   Because the WAL records writes sequentially, it allows the database to perfectly order all transactions. 
*   As each transaction hits the write-ahead log, it is assigned a monotonically increasing sequence number (e.g., T1, T2, T3). 

**The Solution: Snapshot Isolation**
*   **Definition:** Snapshot isolation is a concept that allows a long read to view a completely consistent "snapshot" of the database exactly as it existed at a specific transaction time.
*   **Retaining Old Data:** To achieve this, the database stops deleting old values when a row is overwritten. Instead, it stores all historical values alongside the specific transaction number that wrote them. Since it only stores values for the transactions that actually modify the rows, it does not require an excessive amount of extra storage.
*   **Reading a Snapshot:** If a query wants to see the database snapshot exactly as it was at transaction 15 (T15), it will draw a hard line and completely ignore any writes from transactions that occurred after T15, such as T17 or T22. 
*   For every row, the database simply retrieves the most recent value that was written prior to, or exactly at, T15. 
*   If a row was written at T12 and overwritten at T17, the T15 snapshot will return the T12 value. If a row was entirely created at T22, the T15 snapshot will view that row as non-existent.

**Conclusion**
*   By retaining older versions of values rather than immediately overwriting them, databases ensure that massive, time-consuming reads can execute against a frozen, consistent state of the data without being corrupted by new writes interjecting.
