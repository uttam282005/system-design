# System Design Notes: Single Leader Replication

## 1. Introduction to Single Leader Replication

Single leader replication is a fundamental **replication schema** used to scale databases, particularly for architectures experiencing heavy read traffic.

* **The Baseline Setup:** This architecture assumes the use of **asynchronous replication** [[01:18](http://www.youtube.com/watch?v=8h-a7TsXw28&t=78)]. Synchronous replication is rarely used in practice (99% of the time it is avoided) because it introduces massive latency to ensure there are absolutely zero stale reads [[01:25](http://www.youtube.com/watch?v=8h-a7TsXw28&t=85)].
* **The Core Rule:** * **All writes** must go directly to a single designated **Leader database** [[01:59](http://www.youtube.com/watch?v=8h-a7TsXw28&t=119)].
* **Reads** can be performed by clients from the leader or from any of the available **Follower databases** (replicas) [[02:10](http://www.youtube.com/watch?v=8h-a7TsXw28&t=130)].



### Primary Benefits:

1. **Increased Durability:** Multiple copies of the data exist across different nodes. If a data center goes down, the data remains safe on other replicas [[02:40](http://www.youtube.com/watch?v=8h-a7TsXw28&t=160)].
2. **Increased Read Throughput:** Adding follower databases scales the read performance effectively "for free" because replicas asynchronously copy the data in the background [[02:56](http://www.youtube.com/watch?v=8h-a7TsXw28&t=176)].

---

## 2. Failure Scenarios & Edge Cases

Designing a system around single leader replication requires handling failures gracefully. Depending on which node goes down, the complexity varies drastically.

### Scenario A: Follower Failure (Easy to Fix)

When a follower node goes offline (e.g., a hardware issue or accidental unplug), resolving it is straightforward due to the **Replication Log** [[03:36](http://www.youtube.com/watch?v=8h-a7TsXw28&t=216)].

* **The Mechanism:** Every write sequence is recorded in a sequential log stored on disk [[04:02](http://www.youtube.com/watch?v=8h-a7TsXw28&t=242)]. Each follower tracks the last position it successfully read from this log [[04:17](http://www.youtube.com/watch?v=8h-a7TsXw28&t=257)].
* **The Recovery:** If a follower goes down at log position `50` and the leader advances to position `70`, upon reconnection, the leader simply streams the missing delta (positions `51` to `70`) to the follower [[04:36](http://www.youtube.com/watch?v=8h-a7TsXw28&t=276)]. The follower catches up and is restored to service automatically [[04:51](http://www.youtube.com/watch?v=8h-a7TsXw28&t=291)].

### Scenario B: Leader Failure (Highly Complex)

When a leader goes down, the system must trigger a **failover** (promoting a follower to be the new leader) [[11:13](http://www.youtube.com/watch?v=8h-a7TsXw28&t=673)]. This introduces three massive engineering challenges:

1. **False Positives (Network Issues):** Because networks are asynchronous, it is incredibly difficult to know if a leader is genuinely dead or if there is just a temporary network partition between a specific follower and the leader [[05:58](http://www.youtube.com/watch?v=8h-a7TsXw28&t=358)]. If a single follower incorrectly assumes the leader is dead, trying to replace it prematurely causes chaos [[05:53](http://www.youtube.com/watch?v=8h-a7TsXw28&t=353)].
2. **Data Loss (Lost Writes):** Because data is replicated asynchronously, a leader might process writes up to log position `80` on its local disk and immediately crash before broadcasting those updates [[06:47](http://www.youtube.com/watch?v=8h-a7TsXw28&t=407)]. If the highest up-to-date follower has only seen up to position `70`, and is then promoted to the new leader, writes `71` to `80` are permanently lost into a void [[07:05](http://www.youtube.com/watch?v=8h-a7TsXw28&t=425)]. This creates severe **data integrity issues** [[07:40](http://www.youtube.com/watch?v=8h-a7TsXw28&t=460)].
3. **Split-Brain Scenario:** This occurs when a system mistakenly ends up with **two leaders simultaneously** [[08:12](http://www.youtube.com/watch?v=8h-a7TsXw28&t=492)]. For instance, if an old leader is presumed dead, a new follower is elected as leader, but the old leader suddenly recovers and resumes accepting writes [[08:05](http://www.youtube.com/watch?v=8h-a7TsXw28&t=485)]. Clients will route writes to both nodes indiscriminately, generating conflicting, irreconcilable states across followers. One leader must be aggressively shut down to prevent catastrophic data corruption [[08:46](http://www.youtube.com/watch?v=8h-a7TsXw28&t=526)].

---

## 3. The Need for Distributed Consensus

To solve the dangers of leader failure (like split-brain and incorrect elections), systems require **Distributed Consensus** [[09:22](http://www.youtube.com/watch?v=8h-a7TsXw28&t=562)].

* Consensus algorithms ensure that multiple physically separated computers reach a strict, 100% agreement on system states—specifically, **who the leader is** and **whether that leader is alive or dead** [[09:42](http://www.youtube.com/watch?v=8h-a7TsXw28&t=582)].
* Without distributed consensus, maintaining data correctness across an automated replication setup is virtually impossible [[10:07](http://www.youtube.com/watch?v=8h-a7TsXw28&t=607)].

---

## 4. Summary & Limitations

| Advantages | Limitations |
| --- | --- |
| **Simple Implementation:** Easily scales a single database by attaching read-only followers [[10:30](http://www.youtube.com/watch?v=8h-a7TsXw28&t=630)]. | **No Write Scalability:** Since all writes *must* pass through one single leader node, total system write throughput is fundamentally bottlenecked by that single machine [[11:35](http://www.youtube.com/watch?v=8h-a7TsXw28&t=695)]. |
| **Easy Follower Recovery:** Restoring failed followers via replication logs is trivial [[10:36](http://www.youtube.com/watch?v=8h-a7TsXw28&t=636)]. | **Complex Failovers:** Leader failures introduce risks of split-brain and data loss [[05:08](http://www.youtube.com/watch?v=8h-a7TsXw28&t=308)]. |
| **Geographic Read Scaling:** Replicas can be placed physically closer to localized users (e.g., putting a replica in Australia for Australian users) to minimize read latency [[10:52](http://www.youtube.com/watch?v=8h-a7TsXw28&t=652)]. | **Eventual Consistency:** Reads from asynchronous followers can briefly serve stale data until the background replication catches up. |

*To overcome the write bottleneck of single leader replication, more complex architectures—such as multi-leader or leaderless replication schemas—must be utilized [[11:48](http://www.youtube.com/watch?v=8h-a7TsXw28&t=708)].*
