# System Design Notes: Two-Phase Commit (2PC)

## 1. Why Do We Need Distributed Transactions?

As systems scale and tables are split across separate database partitions or global secondary indexes, single logical events often require updating data across **multiple physically distinct computers** at the same time [[00:45](http://www.youtube.com/watch?v=7DoT2sTGulc&t=45)].

* **The Core Requirement:** These updates demand **Atomicity** (the "A" in ACID) [[01:25](http://www.youtube.com/watch?v=7DoT2sTGulc&t=85)]. Either *all* partition updates succeed, or *none* of them do [[05:26](http://www.youtube.com/watch?v=7DoT2sTGulc&t=326)].
* **The Risk:** If Node 1 writes successfully but Node 2 crashes mid-execution, the cluster falls into a permanently corrupted, **inconsistent state** [[01:32](http://www.youtube.com/watch?v=7DoT2sTGulc&t=92)]. Unlike simple eventual consistency issues which resolve themselves automatically, partial transaction failures persist forever and serve permanently corrupted data to users [[01:44](http://www.youtube.com/watch?v=7DoT2sTGulc&t=104)].

---

## 2. The Two-Phase Commit Protocol Mechanics

The Two-Phase Commit (2PC) protocol solves this by splitting the write process into an explicit **Preparation Phase** and a **Commit Phase**, orchestrated by a central **Coordinator node** (often just an application server or microservice) [[01:57](http://www.youtube.com/watch?v=7DoT2sTGulc&t=117), [02:19](http://www.youtube.com/watch?v=7DoT2sTGulc&t=139)].

### Phase 1: The Prepare Phase (Voting)

1. **The Query:** The Coordinator reaches out to all participating database shards (e.g., Node 1 and Node 2) via network sockets and asks: *"Are you ready to commit this transaction?"* [[02:33](http://www.youtube.com/watch?v=7DoT2sTGulc&t=153), [03:11](http://www.youtube.com/watch?v=7DoT2sTGulc&t=191)]
2. **Local Execution:** Each participant node runs a dry-run local transaction:
* It checks its local Write-Ahead Log (WAL) to ensure it can persist the record [[02:44](http://www.youtube.com/watch?v=7DoT2sTGulc&t=164)].
* It checks for row conflicts and verifies storage constraints [[03:28](http://www.youtube.com/watch?v=7DoT2sTGulc&t=208)].
* **Crucial Step:** It grabs exclusive **row-level locks** on the target records so no other concurrent transaction can touch or modify them [[02:53](http://www.youtube.com/watch?v=7DoT2sTGulc&t=173)].


3. **The Vote:** * If everything checks out locally, the shard responds back to the coordinator with an **`OK` (Yes)** vote [[02:58](http://www.youtube.com/watch?v=7DoT2sTGulc&t=178)]. Once a shard votes `OK`, it enters a blocked state—holding its local locks open and waiting indefinitely for the coordinator's next instruction [[03:04](http://www.youtube.com/watch?v=7DoT2sTGulc&t=184)].
* If a shard encounters an error or space issue, it responds with a **`No` (Abort)** vote [[03:33](http://www.youtube.com/watch?v=7DoT2sTGulc&t=213)].



### Phase 2: The Commit Phase (Execution)

The coordinator evaluates the incoming votes. The protocol then bifurcates based on absolute unanimity:

#### Scenario A: If Any Participant Votes `No` (or Times Out)

If even one node votes `No`, the coordinator writes an **Abort** state into its local disk log and broadcasts an explicit `Abort` command to all participants [[03:40](http://www.youtube.com/watch?v=7DoT2sTGulc&t=220)]. The nodes rollback their partial states, clear their Write-Ahead Logs, and drop their row locks safely [[03:47](http://www.youtube.com/watch?v=7DoT2sTGulc&t=227)].

#### Scenario B: If All Participants Vote `OK`

1. **The Commit Point:** The coordinator writes a definitive `Commit` decision record directly onto its own local disk **Commit Log** [[04:13](http://www.youtube.com/watch?v=7DoT2sTGulc&t=253)]. Once this log is flushed to disk, the transaction is legally committed. If the coordinator crashes a millisecond later, it will read its disk log upon recovery and know it must force the transaction through [[04:23](http://www.youtube.com/watch?v=7DoT2sTGulc&t=263)].
2. **The Broadcast:** The coordinator sends a final `Commit` packet to Node 1 and Node 2 [[04:49](http://www.youtube.com/watch?v=7DoT2sTGulc&t=289)].
3. **Local Application:** The participant shards finalize the transaction, flush the updates permanently to their respective storage disks, **release the row locks**, and reply back with a final execution acknowledgment [[04:58](http://www.youtube.com/watch?v=7DoT2sTGulc&t=298), [05:14](http://www.youtube.com/watch?v=7DoT2sTGulc&t=314)].

---

## 3. Vulnerabilities & Points of Failure

While 2PC technically guarantees absolute transaction atomicity, it is notoriously avoided in high-throughput distributed systems because it possesses **very poor fault tolerance** and introduces severe operational vulnerabilities [[05:38](http://www.youtube.com/watch?v=7DoT2sTGulc&t=338), [07:04](http://www.youtube.com/watch?v=7DoT2sTGulc&t=424)]:

### A. Blocking Coordinator Crash (The Worst-Case Scenario)

If the coordinator node crashes *after* the participants have voted `OK` but *before* broadcasting the final `Commit` or `Abort` command, the entire system grinds to a halt [[05:50](http://www.youtube.com/watch?v=7DoT2sTGulc&t=350)].

* Because the participants voted `OK`, they are bound by the protocol to wait indefinitely for the coordinator's return [[04:34](http://www.youtube.com/watch?v=7DoT2sTGulc&t=274)].
* While waiting, they must **keep all row-level locks open** [[06:01](http://www.youtube.com/watch?v=7DoT2sTGulc&t=361)]. No other client can write or read those locked rows, resulting in cascading system gridlock and thread pool exhaustion across the cluster [[06:08](http://www.youtube.com/watch?v=7DoT2sTGulc&t=368)].

### B. Participant Node Crash

If a participant node goes down *after* voting `OK` but before acknowledging the final commit phase, the coordinator cannot mark the transaction clean [[06:19](http://www.youtube.com/watch?v=7DoT2sTGulc&t=379), [06:41](http://www.youtube.com/watch?v=7DoT2sTGulc&t=401)]. The coordinator is forced to retry sending the network packet **forever** until that specific crashed shard comes back online and successfully acknowledges the commit [[06:51](http://www.youtube.com/watch?v=7DoT2sTGulc&t=411)].

---

## 4. Architectural Summary

| Advantages | Core Architectural Trade-Offs |
| --- | --- |
| **Guaranteed Atomicity:** Ensures complete data consistency across physical cross-partition writes or global secondary indexes [[01:25](http://www.youtube.com/watch?v=7DoT2sTGulc&t=85), [05:26](http://www.youtube.com/watch?v=7DoT2sTGulc&t=326)]. | **High Latency Overhead:** Requires multiple sequential network round-trips over the wire, drastically slowing down basic database write throughput [[10:14](http://www.youtube.com/watch?v=7DoT2sTGulc&t=614)]. |
| **Deterministic Rollbacks:** Cleanly handles local node errors by aborting the entire distributed sequence safely [[03:40](http://www.youtube.com/watch?v=7DoT2sTGulc&t=220)]. | **Availability Nightmare:** A single node failure or network blip during the execution phases can lock up rows across completely healthy nodes indefinitely [[05:50](http://www.youtube.com/watch?v=7DoT2sTGulc&t=350)]. |

* **Engineering Best Practice:** Due to these dangerous blocking issues, engineers designing high-scale cloud platforms try to **avoid cross-partition transactions and global secondary indexes entirely** [[07:29](http://www.youtube.com/watch?v=7DoT2sTGulc&t=449), [07:35](http://www.youtube.com/watch?v=7DoT2sTGulc&t=455)]. Instead, systems prefer denormalization or localizing data scopes within unified NoSQL/document-store records to keep writes localized to a single physical machine boundary [[07:48](http://www.youtube.com/watch?v=7DoT2sTGulc&t=468)].
