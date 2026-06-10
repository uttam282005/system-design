# CAP Theorem: Quick Revision

The CAP theorem states that a distributed system can only simultaneously guarantee **two** of the following three properties:

---

## 1. The Three Properties

*   ### **C**onsistency
    *   **What it is:** Every read receives the most recent write or an error. All nodes in the cluster have the same data at the same time.
    *   **Analogy:** Everyone in a group chat sees the exact same message history.

*   ### **A**vailability
    *   **What it is:** The system always responds to requests, even if some nodes are down. No request is dropped.
    *   **Analogy:** A website stays online and serves content, even if one of its servers fails.

*   ### **P**artition Tolerance
    *   **What it is:** The system continues to operate despite a network partition (a communication break between nodes).
    *   **Analogy:** Two data centers lose connection, but both can still function independently.

---

## 2. The Trade-Off

In any real-world distributed system, network partitions (`P`) are a fact of life. You cannot avoid them. Therefore, the actual trade-off is always between **Consistency (C)** and **Availability (A)** when a partition occurs.

```mermaid
graph TD
    subgraph Distributed System
        A((Node A))
        B((Node B))
    end

    A ---|Network Partition <br> (No Communication)| B

    subgraph "Scenario: Write to Node A"
        direction LR
        W(Write 'X=10') --> A
    end

    subgraph "Choice 1: Prioritize Consistency (CP)"
        direction LR
        R1(Read from B) --> E{Error or Stale Data}
        style E fill:#f9f,stroke:#333,stroke-width:2px
    end
    subgraph "Choice 2: Prioritize Availability (AP)"
        direction LR
        R2(Read from B) --> D(Old Data 'X=5')
        style D fill:#ccf,stroke:#333,stroke-width:2px
    end

    W --> Partition{"Partition Occurs"}
    Partition --> C1{"Choose Consistency (CP)?"}
    Partition --> C2{"Choose Availability (AP)?"}

    C1 --> R1
    C2 --> R2
```

*   **CP (Consistency + Partition Tolerance):**
    *   **Action:** If a partition occurs, the system will stop responding to some requests (become unavailable) to avoid returning inconsistent data.
    *   **Why:** It prefers correctness over being constantly online. The affected partition will wait until communication is restored to sync data and ensure consistency.

*   **AP (Availability + Partition Tolerance):**
    *   **Action:** If a partition occurs, all nodes remain online and respond to requests. This may result in different nodes returning different versions of the data.
    *   **Why:** It prefers being online over having perfect data consistency. Data will eventually become consistent once the partition is resolved.

---

## 3. When to Choose What?

| Choice | Choose When...                                 | Examples                               | Database Systems |
| :----: | :--------------------------------------------- | :------------------------------------- | :--------------- |
| **CP** | Data correctness is critical and cannot be compromised. | Banking, Financial Services, E-commerce Checkout | MongoDB, Redis   |
| **AP** | High availability is crucial; occasional stale data is acceptable. | Social Media Feeds, Like Counts, Leaderboards | Cassandra, DynamoDB |

**Note:** `CA` (Consistency + Availability) systems exist but are not partition-tolerant. They are typically single-site databases (like a traditional RDBMS) and are not considered truly "distributed" in the context of the CAP theorem.
