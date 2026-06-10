**Write Skew and Phantom Writes in Databases**

**Background: Lost Updates**
*   While the focus is on write skew and phantoms, another common database race condition is "lost updates". 
*   An example of a lost update is when two users try to add 1 to a counter simultaneously, but because they both read the same old value, one of the increments is lost. This is typically solved through logging.

**Race Condition 1: Write Skew**
*   **Definition:** Write skew is a race condition where multiple concurrent transactions modify different rows, but the combination of these separate writes violates a broader database invariant.
*   **Example:** Imagine an emergency room database with a strict invariant: there must always be at least one active doctor. Currently, Dr. Oz and Dr. Toboggan are both active. Concurrently, both doctors decide to take a break and update their status to "inactive". Because they are modifying different rows, both writes succeed, leaving zero active doctors and breaking the invariant.
*   **The Locking Problem:** Standard row-level locks do not fix write skew. When Dr. Oz sets himself to inactive, he only grabs the lock for his own row. He does not need the lock for Dr. Toboggan's row. Therefore, both transactions can grab their respective locks simultaneously without blocking each other. 
*   **Solution:** To prevent write skew, a transaction must lock *all* rows that are relevant to the invariant, even if it is not modifying them all. In the emergency room example, a doctor wanting to go inactive would need to grab the locks for *all* currently active doctors. If two doctors try to go offline at the exact same time, they will both try to grab the same locks, and only one transaction will win.

**Race Condition 2: Phantom Writes**
*   **Definition:** Phantoms occur when concurrent transactions attempt to write entirely new rows that conflict with each other, leading to a broken invariant.
*   **Example:** Two customers, Jordan and Trump, are using a bakery database and simultaneously try to claim a cupcake. Both users insert a new row into the database claiming the cupcake along with their email addresses. 
*   **The Locking Problem:** Standard locking fails here because the row they are trying to lock does not physically exist yet. Because there are no existing rows to place locks on, both users successfully write their new rows, resulting in two people claiming the same item.
*   **Solution: Materializing Conflicts:** Phantoms can be prevented by a technique called "materializing conflicts". This involves pre-populating rows in the database for every possible item or scenario that could cause a conflict.
*   **How it Works:** By pre-populating the inventory (e.g., creating empty rows for cookies, pies, and cupcakes in advance), the database generates corresponding lock objects for those items. Now, when Jordan and Trump attempt to claim the cupcake, they must first compete to grab the lock on the pre-existing cupcake row. The first one to grab the lock gets to write their name and email, while the other user is blocked, effectively preserving the invariant.
