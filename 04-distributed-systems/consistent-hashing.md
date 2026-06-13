# System Design Notes: Consistent Hashing & Rebalancing Partitions

## 1. The Core Problem: The Rehashing Bottleneck

When a database grows too large, it must be partitioned (sharded) across multiple nodes. The most straightforward way to route keys evenly across $N$ database nodes is using a standard modulo hash function:

$$\text{Node ID} = \text{Hash}(\text{Key}) \pmod N$$

While highly effective at evenly distributing data across an active cluster, this approach creates a massive failure mode when scaling dynamically:

* **The Scale Problem:** If your cluster has 10 nodes ($N=10$) and you need to add an 11th node ($N=11$) to handle increased load, the mathematical formula changes globally.
* **The Impact:** When recalculating $\text{Hash}(\text{Key}) \pmod{11}$, nearly **99% of all existing keys will suddenly hash to a completely different Node ID**.
* **The Catastrophe:** This triggers the **Rehashing Problem**. The database must move almost its entire dataset across the network to rebalance the cluster, causing immense network overhead, severe disk I/O bottlenecks, and massive cache eviction cycles if used in a caching tier like Memcached or Redis.

---

## 2. The Solution: Consistent Hashing Mechanics

Consistent Hashing completely decouples key placement from the absolute number of nodes in the cluster. It maps both the **Database Nodes** and the **Data Keys** onto a uniform, theoretical mathematical circle known as the **Hash Ring**.

### The Topology Workflow:

1. **The Ring Spectrum:** Assume your hash function (like MD5 or SHA-1) outputs an integer within a fixed range, such as $0$ to $2^{32}-1$. The start and end positions wrap around to form a circular ring.
2. **Mapping Nodes:** Instead of picking nodes by index, you run the unique identifier of the machines (such as their IP addresses or server hostnames) through the hash function: $\text{Hash}(\text{Server IP})$. The resulting numbers dictate exactly where the servers live as physical markers on the perimeter of the Hash Ring.
3. **Mapping Keys:** When a client writes an object, it hashes the object's primary key: $\text{Hash}(\text{Key})$. This assigns the data entry a precise point coordinate on the same ring.
4. **The Lookup Rule (Clockwise Routing):** To determine which node owns a key, the system locates the key’s position on the ring and travels **clockwise** until it bumps into the very first available database server marker. That node is legally designated to store that key.

---

## 3. Graceful Scaling: Adding and Removing Nodes

The primary asset of Consistent Hashing is its ability to localize changes when nodes enter or leave the topology:

### Adding a Node:

* Suppose Node New is inserted into the ring between Node A and Node B.
* Traveling clockwise, the only keys impacted are the entries that live in the slice of the ring immediately *before* Node New but *after* Node A.
* Rather than reshuffling the entire cluster, **only a fraction of the total keys ($\approx 1/N$) need to migrate** from Node B to Node New. The rest of the cluster remains untouched.

### Removing/Failing a Node:

* If Node B crashes or is taken offline, its immediate clockwise neighbor, Node C, inherits its data range automatically.
* Clients looking for keys previously stored on Node B naturally move clockwise past the dead node marker and read from Node C. Only the keys assigned to Node B are impacted or lost.

---

## 4. Resolving Unbalanced Skew: Virtual Nodes

While basic consistent hashing works theoretically, placing raw server hashes randomly onto a circle creates severe data imbalances known as **hotspots** or skewed distributions. If Server A and Server B happen to hash close together, Server B might inherit a massive chunk of the ring's circumference, forcing it to handle 80% of the system traffic while Server A sits idle.

To fix this structural imbalance, systems use **Virtual Nodes (vnodes)**:

* **How it works:** Instead of mapping a physical machine to exactly one coordinate point on the ring, the system assigns **hundreds of virtual tokens per physical server** across the circle perimeter.
* **The Strategy:** For Physical Node A, you compute hashes for `NodeA#1`, `NodeA#2`, `NodeA#3`, up to `NodeA#200`.
* **The Impact:** This fractures Node A's presence into hundreds of interleaved positions scattered uniformly across the entire ring circumference.
* **The Result:** 1.  **Uniform Distribution:** Because the virtual markers are blended evenly, the statistical probability of a single node getting overloaded disappears. Each machine handles an equal share of data blocks.
2.  **Heterogeneous Scaling:** If Server A is an expensive, high-spec machine with twice the RAM of Server B, you can simply allocate it double the number of virtual nodes (e.g., 400 tokens for A, 200 tokens for B) so it takes on a proportionally larger share of the workload.

---

## 5. Summary Architectural Reference

| Metric | Modulo-Based Partitioning ($K \pmod N$) | Consistent Hashing Ring (with Vnodes) |
| --- | --- | --- |
| **Data Migration on Scale Up** | **Catastrophic ($\approx 99\%$):** Nearly the entire cluster must move data rows over the network. | **Minimal ($\approx 1/N$):** Only the keys directly adjacent to the new node are migrated. |
| **Node Failure Resilience** | Poor. Modulo value changes, instantly corrupting key routing calculations. | Excellent. Clockwise neighbor automatically absorbs the traffic block. |
| **Traffic Balancing** | Even by default, but completely non-configurable for varying server hardware specs. | Highly configurable. Virtual node densities adjust dynamically to match physical hardware. |
| **Primary Use Cases** | Static clusters with fixed node counts where scaling events are rare. | High-scale, dynamic environments like distributed databases (Cassandra) or web proxy caching tiers. |

---

For a deeper visual understanding of data placement, ring mechanics, and real-world trade-offs in distributed clusters, see the explanation on [Consistent Hashing and the Rehashing Problem](https://www.youtube.com/watch?v=T4tMuUzxwN8). This video breaks down the core structural challenges that arise when scaling database clusters dynamically before introducing the hash ring solution.
