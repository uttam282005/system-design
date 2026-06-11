# System Design Notes: Dealing with Write Conflicts

## 1. Defining Concurrency in Distributed Systems

In a multi-leader replication setup, multiple users can write to different leader nodes simultaneously. This introduces the risk of **write conflicts** [[00:50](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=50)].

* **What makes rights concurrent?** Two operations are concurrent if they occur without knowledge of each other [[02:41](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=161)]. Concurrency is **not** strictly about two events happening at the exact same physical millisecond; rather, it means that at the time of execution, Node A had not yet processed the state of Node B, and vice versa [[02:52](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=172)].
* **The Naive Merge Failure:** If two clients concurrently increment a value on separate leaders, a naive merge logic like *"keep the highest number"* fails [[01:59](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=119)]. If Node A has a count of `3` and Node B has a count of `5`, simply updating Node A to `5` ignores the 3 increments that occurred on Node A [[02:05](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=125)]. The globally correct state should be the sum of both: `8` [[02:34](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=154)].

---

## 2. Version Vectors: Tracking Causality

To accurately identify whether data updates are sequential or concurrent, systems use a tracking mechanism called a **Version Vector** [[03:09](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=189)].

* **How it works:** Instead of maintaining a single global integer, a version vector acts as an array or map where each index maps to a specific leader node [[03:28](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=208)]. The value at that index tracks the exact number of increments or modifications that specific node has processed [[03:34](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=214)].
* **The State Representation:** For a two-leader system, a version vector of `[3, 0]` means the left leader has handled 3 writes locally and has seen 0 writes from the right leader [[03:40](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=220)]. Conversely, the right leader might hold `[0, 5]` [[03:54](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=234)].
* **The Merge Protocol:** When leaders sync asynchronously, they exchange both their local counter value and their version vector [[04:01](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=241)]. The nodes merge the vectors by taking the highest value for each index [[04:18](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=258)].
* *Example:* Merging `[3, 0]` and `[0, 5]` yields a unified vector of `[3, 5]` [[04:18](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=258)]. The total state value is calculated by summing the internal elements of that vector (`3 + 5 = 8`) [[04:35](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=275)].



---

## 3. Detecting Conflicts via Vector Comparisons

By evaluating version vectors across an all-to-all topology, a database can mathematically determine the relationship between any two writes [[06:00](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=360)].

### Scenario A: Dominant (Sequential) Rights

If every individual index in Version Vector $V_1$ is strictly greater than or equal to ($\ge$) every corresponding index in Version Vector $V_2$, then $V_1$ dominates $V_2$ [[07:31](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=451)].

* *Example:* $V_1 = [2, 1, 2]$ and $V_2 = [1, 1, 1]$. Because $2 \ge 1$, $1 \ge 1$, and $2 \ge 1$, the node holding $V_1$ can safely disregard the message containing $V_2$ [[07:03](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=423)]. It knows with certainty that $V_1$ represents a later state that has already cleanly incorporated all historical mutations tracked by $V_2$ [[07:10](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=430)].

### Scenario B: Interleaving (Concurrent) Rights

If the vectors contain overlapping or contrasting maximums—meaning one vector has seen more writes from Leader 1 but fewer writes from Leader 2—the writes are **interleaving** [[08:23](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=503)].

* *Example:* $V_1 = [1, 0]$ (Update: *Jordan = cute*) and $V_2 = [0, 1]$ (Update: *Jordan = scary*) [[08:42](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=522)].
* Comparing the indexes shows $1 > 0$ but $0 < 1$ [[09:21](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=561)]. Because neither vector strictly dominates the other, the database flags them as genuinely **concurrent**, registering a definitive write conflict [[09:37](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=577)].

---

## 4. Conflict Resolution Strategies

Once a concurrent write conflict is detected via version vectors, the system must reconcile the fork in data state using one of two advanced patterns:

### Strategy 1: Storing Siblings (Application-Assisted Resolution)

Instead of forcing the database engine to pick an arbitrary winner and risk silent data loss, the cluster preserves both conflicting values simultaneously as **siblings** [[09:42](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=582)].

1. **Storage State:** For the key `Jordan`, the database writes a multi-value record containing both elements: `[cute, scary]` [[09:48](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=588)].
2. **The Read Prompt:** The next time any client issues a read request for the key `Jordan`, the database serves back all available siblings [[10:01](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=601)].
3. **Application Uplift:** The conflict resolution responsibility is shifted up to the application layer or the end user [[10:08](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=608)]. The UI can display a prompt asking the user to manually select the correct value [[10:13](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=613)]. Once resolved, the chosen value is written back, purging the stale sibling records [[10:18](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=618)].

### Strategy 2: Conflict-Free Replicated Data Types (CRDTs)

To avoid bothering the user or adding complex code to the application layer, databases can utilize **CRDTs** [[10:32](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=632)].

* CRDTs are specialized, mathematical data structures designed to auto-merge concurrent writes natively within the database without generating friction [[10:39](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=639)].
* By leveraging commutative and associative properties, structures like **distributed counters** (which rely on the version vector addition principles shown above) or specialized **sets** can blend concurrent updates seamlessly behind the scenes [[10:56](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=656)].

---

## 5. Summary Technical Reference

| Feature | Timestamp-Based Resolution (LWW) | Vector-Based Resolution (Siblings/CRDTs) |
| --- | --- | --- |
| **Mechanics** | Checks physical system clocks [[00:56](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=56)]. | Checks logical operational counters per node [[03:28](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=208)]. |
| **Conflict Handling** | Overwrites data based on clock timings [[09:12](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=552)]. | Detects concurrency mathematically via interleaved values [[08:23](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=503)]. |
| **Data Safety** | High risk of data deletion due to clock skew/NTP resets. | Zero data loss; retains updates as siblings or merges natively via CRDTs [[09:42](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=582)]. |
| **Drawback** | Unreliable ordering in asynchronous networks. | Increased storage overhead due to tracking vector metadata per write [[05:41](http://www.youtube.com/watch?v=sa4BJAFT8sU&t=341)]. |
|  |  |  |
