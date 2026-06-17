## 1. What is Stream Processing?

Traditional data pipelines pass information asynchronously by writing to a core database and pulling from it later. **Stream Processing** focuses on capturing, mutating, and reacting to unbounded continuous flows of data—known as **Events**—in real-time as they occur.

### The Fundamental Pattern

* **Producer:** An entity (such as an application server, user client, or hardware sensor) that emits state change updates.
* **Consumer:** An downstream application service designed to intercept, parse, and execute logic based on incoming data blocks.

### The Message Broker Middleware Paradigm

Connecting multiple producers directly to multiple consumers via unique TCP links generates a network connection complexity of **$\mathcal{O}(N^2)$**, overwhelming host hardware.

To scale linearly, architectures insert a centralized **Event/Message Broker** (e.g., Apache Kafka, RabbitMQ) into the middle of the system. The broker acts as a buffer, allowing producers and consumers to decouple and scale independently.

---

## 2. Core Stream Processing Applications

---

### Application A: Time Windowing

When aggregating streaming metrics or logs, consumers bucket continuous data packets into discrete time boundaries:

#### 1. Tumbling Windows

* Non-overlapping time blocks fixed at uniform intervals (e.g., exactly every 60 seconds).
* **Mechanics:** The consumer extracts timestamps from incoming events and groups them using an in-memory map structure (e.g., a hashmap bucketed by the precise minute integer).

#### 2. Hopping Windows

* Overlapping time blocks that measure fixed durations but advance ("hop") forward by a smaller step increment (e.g., a 5-minute window that recalculates every 1 minute).
* **Mechanics:** To avoid redundant processing, a 5-minute hopping window is constructed by aggregating 5 distinct, sequential 1-minute tumbling windows.

#### 3. Sliding Windows

* Fluid, rolling time scopes tracking the absolute "last $X$ minutes" relative to the current time.
* **Mechanics:** Managed via an in-memory queue or linked list. As new events enter the tail of the list, a separate thread drops expired nodes from the head of the list.

---

### Application B: Change Data Capture (CDC)

CDC replicates structural modifications made within a primary database down to secondary, downstream datastores out-of-band.

#### The CDC Workflow

1. A client application executes a write or update directly to the primary database (the system's core source of truth).
2. The database logs the transaction. A CDC daemon reads this log stream and publishes the raw transaction footprint into the message broker.
3. The broker streams the update event to a downstream worker, which applies the change to a secondary target, such as a **Search Index** (e.g., Elasticsearch).

*System Design Advantage:* By using a broker to keep a search index in sync, you avoid using heavy, slow **Two-Phase Commits (2PC)** across unrelated database engines.

---

### Application C: Event Sourcing

Event Sourcing flips the classic database paradigm: instead of saving the final state of an object, the system records the raw **chronological sequence of actions** that created that state.

#### The Event Sourcing Workflow

1. User actions (e.g., `Click_Cart`, `Change_Quantity_To_2`) bypass the database and are sent as schema-agnostic events directly into the message broker.
2. The broker streams these agnostic event blocks down to a consumer, which parses them and saves them to a database.

#### CDC vs. Event Sourcing

* **Change Data Capture:** The message payload is tightly bound to a specific database's internal state layout.
* **Event Sourcing:** The message payload is entirely database-agnostic.
* *Advantage:* If you migrate your core technology stack 10 years later, you can replay the immutable log of raw historical events from the broker to rebuild your application state from scratch in any new data schema.



---

## 3. Achieving "Exactly-Once" Processing

Processing messages precisely once requires combining **At-Least-Once** and **At-Most-Once** engineering invariants.

### Guaranteeing At-Least-Once Delivery

The system must guarantee that a message will successfully reach its destination consumer, even if hardware crashes occur mid-flight.

* **Broker Durability:** The broker must log messages to a physical disk Write-Ahead Log (WAL) and replicate them across separate machines to withstand a node failure.
* **Consumer Acknowledgements (ACKs):** The broker retains a message in its queue until the target consumer returns an explicit ACK token confirming processing is complete. If the consumer crashes before emitting an ACK, the broker re-delivers the message.

### Guaranteeing At-Most-Once Delivery (Preventing Duplicates)

Because network dropouts can cause ACK tokens to get lost, a consumer might safely process an event but crash right before notifying the broker. The broker will then re-deliver that identical message, creating a duplicate. Systems handle this in one of two ways:

#### Approach 1: The Two-Phase Commit (2PC) Method

A central coordinator runs a 2PC protocol across the broker and consumer database simultaneously. It asks the broker if it can safely pop the message and asks the consumer if it can safely write the record. If both agree, the transaction commits.

* *Trade-off:* Avoid this where possible; 2PC is slow and heavily bottlenecks streaming ingestion throughput.

#### Approach 2: The Idempotent Consumer Method

The system ensures that processing a duplicate message multiple times produces no additional side effects (e.g., re-running `SetBalance = 100` is safe, whereas `AddBalance += 100` is dangerous).

* **The Unique Key Pattern:** Every message is tagged with a unique cryptographic ID. The consumer stores processed IDs in an internal database table. When a message arrives, the consumer checks this table; if the ID exists, it returns a success token and discards the duplicate.
* *The Partitioning Requirement:* This pattern assumes strict data routing. If two separate consumer threads read the same message from the broker concurrently, they may both check the lookup table before either has finished writing the key, causing duplicate processing. Stream partitioning must enforce that specific message keys map exclusively to a single consumer instance.

---

## 4. Architectural Summary Matrix

| Design Concept | Engineering Function | Primary Use Case |
| --- | --- | --- |
| **Message Broker** | Centralizes $\mathcal{O}(N^2)$ link paths into linear loops | High-scale pub/sub application decoupling |
| **Tumbling Window** | Non-overlapping fixed time segment groupings | Rolling minute/hour system metric reporting |
| **Change Data Capture** | Streams transactional database updates out-of-band | Bypassing 2PC to sync secondary indexes/caches |
| **Event Sourcing** | Records atomic lifecycle events sequentially | Decoupling application business logic from data storage |
| **Idempotent Keys** | Filters out duplicate re-deliveries via tracking lists | Ensuring safe data mutation over faulty network paths |
| **Stream Partitioning** | Routes specific message keys to unique consumer threads | Preventing race conditions during idempotent key checks |
