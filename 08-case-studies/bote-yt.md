
Exactly. This is the part of the system-design interview where you turn the architecture into **numbers** and then use those numbers to justify your design.

I'll make reasonable assumptions and show the math.

## 1. Given

* **100M DAU**
* **10M uploads/day**
* Average video size = **500 MB**
* **1M concurrent viewers**

I'll additionally assume:

* Average video duration = **10 minutes**
* Average playback bitrate = **5 Mbps**
* Peak traffic ≈ **5× average**
* 5 output resolutions/bitrates
* CDN cache hit rate ≈ **95%**

---

# 2. S3 storage

Each day:

```text
10M videos × 500 MB
= 5,000,000,000 MB
= 5 PB/day
```

So raw original uploads:

```text
5 PB/day
× 30
= 150 PB/month
```

That's **150 PB/month of original videos**.

But we also have transcoded versions.

Suppose the total size of all transcoded representations is roughly **1.5× the original**:

```text
Original:       5 PB/day
Transcoded:   7.5 PB/day
-----------------------
Total:        12.5 PB/day
```

So approximately:

```text
12.5 PB × 30 = 375 PB/month
```

That's why at this scale you'd also think about:

* lifecycle policies
* cheaper storage tiers
* deleting failed/incomplete uploads
* possibly retaining originals in colder storage

---

# 3. Upload bandwidth

We're ingesting:

```text
5 PB/day
```

Convert to bandwidth:

```text
5 × 10^15 bytes / 86,400 sec
≈ 57.9 GB/s
```

In bits:

```text
57.9 GB/s × 8
≈ 463 Gbps
```

So the **average incoming bandwidth is ~460 Gbps**.

If peak is 5×:

```text
≈ 2.3 Tbps
```

This is a huge reason why:

> **Client → S3 directly**

is important.

You absolutely don't want:

```text
Client → API servers → S3
```

for 500 MB videos.

---

# 4. Viewer bandwidth

Now the more interesting number.

We have:

```text
1M concurrent viewers
```

Assume average bitrate:

```text
5 Mbps/viewer
```

Therefore:

```text
1M × 5 Mbps
= 5,000,000 Mbps
= 5 Tbps
```

So the CDN needs to serve approximately:

> **5 Tbps**

of sustained video traffic.

In bytes:

```text
5 Tbps / 8
= 625 GB/s
```

Per day:

```text
625 GB/s × 86,400
≈ 54 PB/day
```

So:

> **~54 PB/day of video delivery**

or roughly:

```text
54 × 30 = 1.62 EB/month
```

That's enormous.

And this is exactly why **CDN is absolutely essential**.

---

# 5. What does the CDN save us?

Suppose CDN cache hit rate = **95%**.

Then only 5% reaches S3:

```text
54 PB/day × 5%
= 2.7 PB/day
```

Without CDN:

```text
S3 → ~54 PB/day
```

With 95% cache hit:

```text
S3 → ~2.7 PB/day
```

That's a **20× reduction in origin traffic**.

This is one of the strongest arguments for putting a CDN in front of S3.

---

# 6. DB QPS

This one requires an assumption because **100M DAU doesn't tell us how many requests each user generates**.

Suppose each user generates:

```text
10 video-related requests/day
```

Then:

```text
100M × 10
= 1 billion requests/day
```

Average QPS:

```text
1B / 86,400
≈ 11,600 QPS
```

Peak at 5×:

```text
≈ 58,000 QPS
```

So you're looking at roughly:

> **12K average QPS / 60K peak QPS**

for video-related application requests.

But **don't send all of this to one relational DB**.

You'd likely have:

```text
                    API
                     │
             ┌───────┴────────┐
             │                │
          Cache             DB
             │
       Redis/Memcached
```

And separate workloads:

```text
PostgreSQL/MySQL
    │
    ├── users
    ├── video metadata
    ├── comments
    └── likes

Search index
    │
    └── video search

Analytics/event store
    │
    └── views/watch history
```

---

# 7. Views are a special problem

Imagine every viewer generates one view event.

That's:

```text
1M concurrent viewers
```

But concurrent viewers aren't daily viewers.

Suppose we eventually get **500M video views/day**:

```text
500M / 86,400
≈ 5,800 events/sec
```

Peak could easily be:

```text
~30K events/sec
```

You don't want:

```sql
UPDATE videos
SET view_count = view_count + 1
WHERE video_id = ?
```

for every view.

Instead:

```text
Client
  │
  ▼
API
  │
  ▼
Kafka / Queue
  │
  ▼
View aggregation workers
  │
  ▼
Redis / DB
```

Workers can aggregate:

```text
video123: +10,000 views
video456: +4,200 views
...
```

and periodically update the persistent database.

---

# 8. Transcoding capacity

This is where you need to make an explicit assumption.

10M uploads/day.

Average video:

```text
10 minutes
```

Therefore:

```text
10M × 10 min
= 100M minutes/day
```

or:

```text
100M / 60
≈ 1.67M video-hours/day
```

Now suppose we generate **5 representations**:

```text
144p
360p
480p
720p
1080p
```

Total processing:

```text
1.67M video-hours × 5
≈ 8.33M representation-hours/day
```

If one transcoding worker can process video at approximately **1× realtime**:

```text
8.33M worker-hours/day
```

Divide by 24:

```text
8.33M / 24
≈ 347,000 workers
```

That's obviously enormous.

But this illustrates something important:

> **You cannot reason about transcoding capacity without knowing the transcoding speed per worker.**

Suppose optimized hardware processes video at **20× realtime**:

```text
347,000 / 20
≈ 17,350 workers
```

At **50× realtime**:

```text
347,000 / 50
≈ 6,940 workers
```

So you'd build a large **autoscaling transcoding fleet**, potentially using specialized compute.

---

# 9. Queue size

10M uploads/day:

```text
10M / 86,400
≈ 116 uploads/sec
```

Average upload-processing jobs:

> **~116 jobs/sec**

Peak at 5×:

> **~580 jobs/sec**

But each upload generates multiple transcoding tasks.

If 5 renditions:

```text
116 × 5
≈ 580 transcoding jobs/sec
```

Peak:

```text
580 × 5
≈ 2,900 jobs/sec
```

So your queue needs to comfortably absorb **thousands of processing jobs/sec**.

And that's exactly why you don't synchronously process videos inside the upload request.

---

# 10. Putting the numbers together

| Metric                          |          Estimate |
| ------------------------------- | ----------------: |
| Users                           |          100M DAU |
| Uploads                         |           10M/day |
| Raw upload storage              |      **5 PB/day** |
| Raw storage/month               |  **150 PB/month** |
| Total storage incl. transcoding | ~**375 PB/month** |
| Average upload bandwidth        |     **~460 Gbps** |
| Peak upload bandwidth           |     **~2.3 Tbps** |
| Concurrent viewers              |                1M |
| Playback bitrate                |            5 Mbps |
| CDN bandwidth                   |       **~5 Tbps** |
| CDN traffic                     |    **~54 PB/day** |
| Origin traffic @ 95% cache hit  |   **~2.7 PB/day** |
| App requests                    |          ~1B/day* |
| Average API QPS                 |         **~12K*** |
| Peak API QPS                    |         **~60K*** |
| Upload jobs                     |          ~116/sec |
| Peak upload jobs                |          ~580/sec |
| Transcoding jobs                | ~580/sec average* |

*These depend on assumptions.

---

# The architectural consequences

This is the important part. **Don't just calculate numbers—use them to make architectural decisions.**

### 460 Gbps upload

→ Direct-to-S3 multipart upload.

```text
Client ───────────────► S3
```

Not:

```text
Client → API → S3
```

### 5 Tbps video delivery

→ CDN is mandatory.

```text
Client → CDN → S3
```

### Thousands of transcoding jobs/sec

→ Queue + horizontally scalable workers.

```text
S3
 ↓
Event
 ↓
Queue
 ↓
Transcoding Workers
```

### 12K+ API QPS

→ Load balancer + stateless API servers + caching.

```text
             ┌── API ──┐
Client → LB ─┼── API ──┼→ Redis
             ├── API ──┤
             └── API ──┘
                    │
                    ▼
                   DB
```

### Huge view volume

→ Don't synchronously increment a DB row per view.

```text
Views → Kafka → Aggregator → DB
```

### Huge search workload

→ Don't query PostgreSQL with `LIKE`.

```text
Video Service
     │
     ▼
   Queue
     │
     ▼
Search Index
```

---

## The interview lesson

The strongest part of this design isn't saying:

> "I'll use S3, Kafka, Redis and a CDN."

Anyone can memorize that.

The stronger answer is:

> "10M uploads × 500 MB gives us 5 PB/day of raw ingestion, which means roughly 460 Gbps average ingress. Therefore uploads should bypass our API servers and use multipart presigned S3 uploads. Meanwhile, 1M concurrent viewers at 5 Mbps creates roughly 5 Tbps of egress, so video delivery must go through a CDN. Since transcoding is expensive and asynchronous, S3 events feed a queue and horizontally scalable workers. Finally, high-frequency events such as views shouldn't synchronously update database rows."

**That's the level of reasoning you should aim for in system-design interviews.**
