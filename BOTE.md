
For **back-of-the-envelope (BOTE) estimations in system design**, you don't need to memorize hundreds of numbers. Memorize a compact set of **orders of magnitude, conversions, and system limits**.

## 1. Time conversions — memorize these

| Unit     |            Approximation |
| -------- | -----------------------: |
| 1 minute |               **60 sec** |
| 1 hour   |    **3,600 sec ≈ 4,000** |
| 1 day    | **86,400 sec ≈ 100,000** |
| 1 month  |             **2.5M sec** |
| 1 year   |      **31.5M sec ≈ 30M** |

### Very useful shortcut

**1 day ≈ 100K seconds**

So:

* 1M requests/day → **~10 RPS**
* 10M/day → **~100 RPS**
* 100M/day → **~1K RPS**
* 1B/day → **~10K RPS**

This is one of the most useful numbers in system design.

---

# 2. Number conversions

Memorize:

| Number     | Approx |
| ---------- | -----: |
| 1 thousand |    10³ |
| 1 million  |    10⁶ |
| 1 billion  |    10⁹ |
| 1 trillion |   10¹² |

And:

**1 billion / day ≈ 11.6K requests/sec**

So roughly:

> **1B/day ≈ 10K RPS**

---

# 3. Storage units

Know these by heart:

| Unit | Bytes |
| ---- | ----: |
| 1 KB |   10³ |
| 1 MB |   10⁶ |
| 1 GB |   10⁹ |
| 1 TB |  10¹² |
| 1 PB |  10¹⁵ |

For BOTE, **decimal units are easier**.

### Example

Suppose:

* 10M users
* each generates 10 records/day
* each record = 1 KB

Then:

10M × 10 × 1 KB

= 100M KB

≈ **100 GB/day**

Annual:

100 GB × 365 ≈ **36.5 TB/year**

---

# 4. Data-rate conversions

Very important:

**8 bits = 1 byte**

Therefore:

* 1 Gbps = **125 MB/s**
* 100 Mbps = **12.5 MB/s**
* 10 Mbps = **1.25 MB/s**
* 1 Mbps = **125 KB/s**

Formula:

> **MB/s = Mbps ÷ 8**

---

# 5. Latency numbers

You don't need exact numbers. Know the rough hierarchy.

### CPU / memory

| Operation     |    Rough order |
| ------------- | -------------: |
| CPU operation |        **~ns** |
| L1 cache      |      **~1 ns** |
| L2 cache      |      **~5 ns** |
| RAM           | **~50–100 ns** |
| SSD access    |    **~100 µs** |
| HDD access    |   **~1–10 ms** |

### Network

| Operation        |    Rough latency |
| ---------------- | ---------------: |
| Same machine     |   **µs or less** |
| Same datacenter  |    **~0.1–1 ms** |
| Same region      |     **~1–10 ms** |
| Cross-region     |   **~50–150 ms** |
| Intercontinental | **~100–300+ ms** |

The exact values aren't important.

The important mental model is:

> **RAM << SSD << network << cross-region network**

---

# 6. Network bandwidth

Typical rough numbers for estimation:

* 1 Gbps server NIC → **125 MB/s**
* 10 Gbps → **1.25 GB/s**
* 100 Gbps → **12.5 GB/s**

This lets you answer questions like:

> "Can one server handle this traffic?"

Example:

100K requests/sec × 100 KB/request

= 10 GB/sec

A 10-Gbps NIC gives only ~1.25 GB/sec.

Therefore you'd need **multiple machines**, even before considering CPU.

---

# 7. QPS/RPS calculation

The fundamental equation:

> **RPS = requests / time**

For daily traffic:

> **RPS ≈ requests/day ÷ 100,000**

Then account for peak traffic.

A common interview assumption:

> **Peak RPS ≈ 2–5 × average RPS**

Example:

100M requests/day

Average:

100M / 100K = **1K RPS**

Assume 3× peak:

**3K peak RPS**

---

# 8. Storage growth formula

Memorize this pattern:

> **Daily storage = writes/day × average object size**

Then:

> **Yearly storage = daily storage × 365**

Example:

1M events/day × 2 KB

= 2 GB/day

≈ **730 GB/year**

Then add replication:

3 replicas:

730 GB × 3 = **2.2 TB**

And indexes/backups if relevant.

---

# 9. Replication

Common assumptions:

### Database

**3 replicas** is a useful default mental model.

For example:

1 TB raw data

× 3 replicas

≈ **3 TB**

But don't blindly apply 3×. State the assumption.

---

# 10. Common object sizes

Useful rough estimates:

| Data               |      Approx size |
| ------------------ | ---------------: |
| Integer            |            4–8 B |
| UUID               |            ~16 B |
| Timestamp          |             ~8 B |
| Short string       |        ~10–100 B |
| User record        |    **~0.5–2 KB** |
| JSON API response  |     **~1–10 KB** |
| Small image        | **~100 KB–1 MB** |
| High-quality image |      **~1–5 MB** |
| Video              |   **100 MB–GBs** |

For interviews, don't obsess over exact values. Say:

> "I'll assume an average user record is 1 KB."

Then calculate.

---

# 11. Server capacity

There is **no universal "one server handles X requests/sec" number**.

It depends on:

* CPU
* request complexity
* database calls
* network
* payload size
* language/runtime
* concurrency

But for BOTE, you can make an explicit assumption.

For example:

> Assume one application server can handle ~1K RPS for this workload.

Then:

10K RPS / 1K RPS

= **10 servers**

Add redundancy/headroom:

**~12–15 servers**

The important thing is **showing your assumption**, not memorizing a magical server capacity.

---

# 12. Availability numbers

Very useful for system design interviews:

| Availability | Downtime/year |
| ------------ | ------------: |
| 99%          |    ~3.65 days |
| 99.9%        |    ~8.8 hours |
| 99.99%       |   ~53 minutes |
| 99.999%      |  ~5.3 minutes |
| 99.9999%     |   ~32 seconds |

Remember:

> **99.9 → ~9 hours**
> **99.99 → ~1 hour**
> **99.999 → ~5 minutes**

---

# 13. Power-of-two storage

Useful when thinking about databases, memory, caches:

|     | Approx |
| --- | -----: |
| 2¹⁰ |     1K |
| 2²⁰ |     1M |
| 2³⁰ |     1B |
| 2⁴⁰ |     1T |

This is particularly useful for:

* hash tables
* memory
* cache sizing
* partition counts
* bitmaps

---

# 14. Cache estimation

The key equation:

> **Cache size = number of cached objects × average object size**

Example:

10M users × 1 KB/user

= **10 GB**

If caching only 10%:

= **1 GB**

For cache hit rate, a reasonable interview assumption might be:

> **80–95%**, depending on workload.

But don't claim a specific hit rate without explaining why.

---

# 15. Queue estimation

Suppose:

* 10K events/sec
* each event = 2 KB

Data ingress:

10K × 2 KB

= **20 MB/sec**

Per day:

20 MB × 86,400

≈ **1.7 TB/day**

This is the kind of calculation you'll frequently do with:

* Kafka
* SQS
* Pub/Sub
* event streams
* logging systems

---

# 16. The most important formulas

You should be able to write these **without thinking**:

### Traffic

**RPS = requests/day ÷ 86,400**

Approx:

**RPS ≈ requests/day ÷ 100K**

### Storage

**Storage = objects × object size**

### Storage growth

**Daily storage = writes/sec × object size × 86,400**

### Bandwidth

**Bandwidth = requests/sec × payload size**

### Number of servers

**servers = total RPS / RPS per server**

### Cache

**cache size = objects × object size**

### Queue backlog

**backlog = arrival rate − processing rate**

More precisely, if the difference is sustained:

**backlog growth/sec = arrival rate − processing rate**

---

# 17. One extremely useful estimation trick

Use **powers of 10**, not exact arithmetic.

Suppose:

> 50M users × 20 requests/day

Don't calculate unnecessarily:

50M × 20 = 1B/day

Then:

**1B/day ≈ 10K RPS**

Done.

For system design interviews, being able to do:

> **50M × 20 → 1B → 10K RPS**

quickly is much more valuable than doing precise calculations.

---

## What I would memorize for interviews

If you want the **minimum set**, memorize these:

1. **1 day ≈ 100K seconds**
2. **1 year ≈ 30M seconds**
3. **1M/day ≈ 10 RPS**
4. **1B/day ≈ 10K RPS**
5. **8 bits = 1 byte**
6. **1 Gbps = 125 MB/s**
7. **1 KB = 10³ bytes**
8. **1 MB = 10⁶ bytes**
9. **1 GB = 10⁹ bytes**
10. **1 TB = 10¹² bytes**
11. **RAM ~100 ns**
12. **SSD ~100 µs**
13. **Network ~ms**
14. **Cross-region ~100 ms**
15. **99.9% ≈ 9 hours/year**
16. **99.99% ≈ 1 hour/year**
17. **99.999% ≈ 5 minutes/year**
18. **Peak traffic ≈ 2–5× average** *(state as an assumption)*
19. **3 replicas** is a common HA assumption
20. Always calculate **traffic → storage → bandwidth → compute → peak → headroom**

The biggest mistake in BOTE is trying to memorize too many "facts." **Memorize the few anchor numbers above and practice deriving everything else from them.**
