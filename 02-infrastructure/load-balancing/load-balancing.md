## 1. Scaling Foundations: Horizontal vs. Vertical

To handle growing traffic volumes, system designs scale their infrastructure using one of two patterns:

* **Vertical Scaling (Scaling Up):** Maximizing the computing power of a single machine node by upgrading its physical hardware components (such as installing a higher-core CPU or expanding system RAM) `[00:00:50]`. While simpler to implement because it avoids distributed systems complexity, vertical scaling is limited by absolute hardware limits and introduces a single point of failure `[00:00:56, 00:01:05]`.
* **Horizontal Scaling (Scaling Out):** Distributing traffic across an elastic cluster of multiple, interconnected commodity machines `[00:01:05, 00:01:13]`. This model offers infinite scaling potential and high fault tolerance, but it requires an intelligent routing layer to manage incoming traffic safely `[00:01:20]`.

---

## 2. The Role of Load Balancers Across the Infrastructure

A **Load Balancer** is the traffic orchestration component required when horizontally scaling out infrastructure `[00:01:42]`. Its primary role is to evenly distribute incoming request volumes across an array of identical target nodes, ensuring no single machine is overwhelmed while others sit starved for work `[00:01:54, 00:01:59]`.

Load balancers are not limited to the edge of a network; they sit between almost every tier of a modern distributed architecture `[00:02:05]`:

1. **Client to Application Server Tier:** Routes incoming user HTTP requests across an array of horizontal application servers `[00:02:10]`.
2. **Application Server to Distributed Caching Tier:** Determines which dedicated memory cache node (e.g., Redis or Memcached shards) should process an app-tier query `[00:02:16, 00:02:22]`.
3. **Caching Tier to Database Tier:** Orchestrates read and write operations across split primary/replica relational or non-relational database nodes `[00:02:22, 00:02:29]`.

---

## 3. Load Balancing Routing Policies

To decide which node receives an incoming request, load balancers execute predefined routing algorithms `[00:02:41, 00:02:53]`.

### Round Robin & Weighted Round Robin

* **Standard Round Robin:** Distributes requests sequentially down the line of available nodes (`Node 1 -> Node 2 -> Node 3 -> Node 1`) `[00:03:00]`. This works perfectly if all backend servers feature identical hardware.
* **Weighted Round Robin:** Adjusts traffic distribution based on a node's hardware capacity `[00:03:08]`. If Node A features twice the CPU capacity of Node B, the load balancer routes two sequential requests to Node A for every one routed to Node B `[00:03:14, 00:03:19]`.

### Lowest Response Time

The load balancer actively tracks performance metrics, keeping a running average of the latency response times across every target server `[00:03:29, 00:03:35]`. New requests are dynamically routed to whichever server is currently responding fastest `[00:03:41, 00:03:47]`. This approach helps equalize performance across the cluster but introduces processing state overhead on the load balancer itself `[00:03:35, 00:03:52]`.

### Hashing: Layer 4 vs. Layer 7 Routing

Hashing policies route traffic by passing request parameters into a mathematical hash function `[00:03:57]`. This is executed at two distinct network depths:

* **Layer 4 (Transport Layer) Load Balancing:** Routes traffic based strictly on protocol and network headers—such as the client’s source IP address and destination port `[00:04:08, 00:04:15]`. It never decodes or reads the underlying application payload, making it exceptionally fast and CPU-efficient `[00:04:21, 00:04:26]`.
* **Layer 7 (Application Layer) Load Balancing:** Deeply inspects and decodes the actual application data payload (such as parsing HTTP headers, cookies, or specific JSON message bodies) `[00:04:35]`. While more CPU-intensive and slower than Layer 4, Layer 7 balancing gives architectures fine-grained routing flexibility—such as redirecting video-streaming requests to a dedicated media-processing cluster `[00:04:40]`.

---

## 4. Maximizing Cache Localized Performance via Consistent Hashing

When horizontally scaled application servers maintain their own **local, in-memory caches**, choosing the right hashing algorithm is critical `[00:05:16, 00:05:22]`.

### The Failure of Modulo Hashing (`Hash(Key) % N`)

If a load balancer routes traffic using a standard modulo formula (`Server ID = Hash(Username) % Total Servers`), a user request is mapped to a predictable node `[00:06:23]`. If the target node fetches data from a database, it caches it locally in RAM `[00:05:46, 00:05:53]`.

However, if a server crashes or a new node is added to handle high traffic, the total number of servers ($N$) changes to $N-1$ or $N+1$ `[00:06:36]`. This slight shift changes the output of the modulo formula for **almost every single key in the system** `[00:06:44]`. On the next request cycle, traffic is routed to entirely different nodes, triggering a **massive, cluster-wide cache wipeout** that floods the backend database `[00:07:41]`.

### The Consistent Hashing Ring Solution

To prevent cache wipeouts during scaling events, load balancers implement a **Consistent Hashing Ring** `[00:06:23, 00:06:51]`.

* **The Mechanics:** Both the unique keys and the physical servers are hashed onto a shared 360-degree virtual token ring `[00:06:51, 00:07:01]`. A key maps to whichever server node is located closest to it moving in a clockwise direction `[00:07:06]`.
* **The Graceful Scaling Advantage:** When a new server (Server 4) is added to the ring, it only intercepts a small fraction of keys from its immediate counter-clockwise neighbor `[00:07:19, 00:07:30]`. The rest of the keys along the ring remain mapped to their original servers `[00:07:35]`. This isolates the cache-miss penalty to a tiny fraction of total traffic, preserving the rest of the cluster's cache hit ratio `[00:07:41, 00:07:46]`.

---

## 5. Fault Tolerance and High Availability Models

Deploying a single load balancer introduces a dangerous Single Point of Failure (SPOF); if it goes offline, the entire application becomes inaccessible `[00:07:57, 00:08:04]`. To ensure high availability, production systems run load balancers in redundant pairs using one of two layouts:

### Layout A: Active-Active Load Balancing

In an **Active-Active configuration**, multiple load balancers run concurrently, sharing the global request load simultaneously `[00:08:15, 00:08:22]`. This configuration maximizes cluster throughput and prevents the load balancer itself from becoming a bottleneck `[00:08:22, 00:08:29]`. However, sharing traffic complicates strategies like Lowest Response Time routing because performance state tracking is split across separate load balancing instances `[00:08:35, 00:08:53]`.

### Layout B: Active-Passive Load Balancing

In an **Active-Passive configuration**, a primary load balancer processes 100% of the live incoming traffic, while a secondary, redundant load balancer sits completely idle `[00:09:04, 00:09:10]`.

```
                  ┌──────────────────────┐
                  │    Apache ZooKeeper  │
                  └───────────┬──────────┘
             (Heartbeat)      │      (Heartbeat)
                  ┌───────────┴──────────┐
                  ▼                      ▼
       ┌────────────────────┐  [Failover]  ┌─────────────────────┐
       │ Active Load Balancer│ ──────────► │Passive Load Balancer│
       └────────────────────┘              └─────────────────────┘
         (Handles Traffic)                    (Promoted to Active)

```

* **Coordination Layer:** Both units are continuously monitored by a highly available distributed consensus system (such as **Apache ZooKeeper**) via lightweight network heartbeats `[00:09:10, 00:09:17]`.
* **Automated Failover Execution:** If the active load balancer suffers a hardware failure, ZooKeeper instantly notices the missing heartbeat `[00:09:17]`. It flags the primary unit as dead and promotes the passive load balancer to active status `[00:09:22]`. The secondary balancer assumes control of the IP address mapping and resumes routing traffic to the backend cluster with minimal disruption `[00:09:22, 00:09:30]`.
