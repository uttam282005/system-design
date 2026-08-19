
This is a **good first-pass system design**, especially for an interview. You have identified the major components—API gateway, object storage, CDN, processing pipeline, queue, and metadata DB. I'd rate it around **7/10 currently**, but there are a few important architectural issues I'd fix before presenting it in an interview.

### What you got right

Your overall flow is sensible:

```text
Client
  │
  ▼
API Gateway
  │
  ▼
Video Service ───────► DB
  │
  ▼
S3 ─────► Queue ─────► Video Processing
                         │
                         ▼
                         S3
                         │
                         ▼
                        CDN
                         │
                         ▼
                       Client
```

Good decisions:

* **S3 for video storage** rather than DB.
* **CDN for video delivery**.
* **Queue between upload and processing** → decouples ingestion from expensive transcoding.
* **Video processing asynchronously**.
* Supporting **multiple formats/resolutions**.
* Thinking about **resumable uploads**.
* Separating **video metadata from video bytes**.
* Accepting some **eventual consistency** for likes/comments/views.

Those are all things I'd expect in a reasonable YouTube design.

---

# The biggest thing I'd change: upload flow

Currently you have:

```text
Client → S3
```

which is good, but your diagram suggests the client uploads directly to S3 after interacting with the video service.

Make this explicit:

```text
Client
   │
   │ 1. POST /videos
   ▼
Video Service
   │
   │ 2. Create video record
   │ 3. Generate multipart presigned URLs
   ▼
Client
   │
   │ 4. Upload chunks directly
   ▼
S3
```

The backend **should not proxy the actual video bytes**.

For a 5 GB video:

```text
BAD:

Client → API Server → S3
             ↑
       5 GB passes through


GOOD:

Client ───────────────────→ S3
         multipart upload
```

This dramatically reduces load on your application servers.

---

# Your resumable upload design should use multipart upload

This is an important interview point.

For example:

```text
Video = 5 GB

        ┌── Part 1 ──┐
        ├── Part 2 ──┤
        ├── Part 3 ──┤
        ├── Part 4 ──┤
        │    ...      │
        └── Part N ──┘
              │
              ▼
             S3
```

Each part can be uploaded independently.

If part 37 fails:

```text
retry part 37
```

instead of:

```text
restart 5 GB upload
```

Your earlier question about **ETag** is actually relevant here: multipart uploads return part-level ETags, which the client can use when completing the multipart upload.

---

# The second major issue: your S3 → processing trigger

You currently have something resembling:

```text
Video Service
      │
      ▼
    Queue
      │
      ▼
Video Processing
```

That's okay, but I'd make the trigger more event-driven:

```text
                 ┌──────────────┐
Client ─────────►│      S3      │
                 └──────┬───────┘
                        │
                  Object Created
                        │
                        ▼
                  Event / Queue
                        │
                        ▼
                Video Processor
```

So after the multipart upload is completed, S3 emits an event.

The processor consumes the event and starts transcoding.

This gives you a cleaner architecture:

```text
S3
 ↓
Event
 ↓
Queue
 ↓
Workers
```

And the queue provides:

* buffering
* retry
* backpressure
* decoupling
* failure recovery

---

# Your video processing design is directionally correct

You wrote:

> split the video
> transcode it into different formats

I'd change the terminology slightly.

You generally want **adaptive bitrate streaming**.

For example:

```text
Original
   │
   ▼
Transcoding
   │
   ├── 144p
   ├── 360p
   ├── 480p
   ├── 720p
   └── 1080p
```

Then segment them:

```text
720p/
 ├── segment001.ts
 ├── segment002.ts
 ├── segment003.ts
 └── manifest.m3u8

1080p/
 ├── segment001.ts
 ├── segment002.ts
 ├── segment003.ts
 └── manifest.m3u8
```

The client can then dynamically switch:

```text
1080p
  ↓ bad network
720p
  ↓
480p
```

without stopping playback.

This is much closer to how YouTube-like systems actually work.

---

# Don't store every segment URL in your DB

You currently have:

> video metadata → add S3 URLs of individual segments

I'd avoid that.

Imagine:

```text
1 video
×
5 resolutions
×
1000 segments
```

That's potentially thousands of URLs associated with one video.

Instead store something like:

```text
video_id
status
duration
title
description
storage_prefix
manifest_location
thumbnail_location
```

For example:

```text
video_id = 123

storage_prefix =
s3://videos/123/

manifest =
s3://videos/123/master.m3u8
```

Your storage structure can be:

```text
videos/
└── 123/
    ├── original/
    ├── 360p/
    ├── 720p/
    ├── 1080p/
    ├── master.m3u8
    └── thumbnails/
```

The **manifest references the segments**.

The DB doesn't need to know about every segment.

---

# CDN should sit in front of S3

Your diagram has:

```text
Client → CDN
```

Good.

But conceptually make the relationship explicit:

```text
                  ┌─────────────┐
                  │     CDN     │
                  └──────┬──────┘
                         │
                      cache miss
                         │
                         ▼
                        S3
```

So:

```text
Client
  │
  ▼
CDN
  │
  ├── cache hit → return segment
  │
  └── cache miss → S3 → cache → client
```

This is critical because YouTube has enormous read traffic.

You don't want:

```text
millions of users
      ↓
     S3
```

You want:

```text
millions of users
      ↓
     CDN
      ↓
     S3
```

---

# Your database model needs some changes

You have:

```text
videos
users
likes
comments
```

That's fine.

I'd model likes approximately as:

```text
likes
------
user_id
video_id
created_at

PRIMARY KEY(user_id, video_id)
```

This gives you an important property:

> A user can only like a video once.

For example:

```sql
PRIMARY KEY (user_id, video_id)
```

Then maintain an aggregate:

```text
videos
------
video_id
like_count
comment_count
view_count
```

But be careful with high traffic.

You don't want millions of concurrent requests doing:

```sql
UPDATE videos
SET view_count = view_count + 1
WHERE video_id = ?
```

because a hugely popular video can become a **hot row**.

For views, I'd use an asynchronous aggregation pipeline:

```text
Client
  │
  ▼
View Event
  │
  ▼
Kafka / Queue
  │
  ▼
Aggregation Workers
  │
  ▼
DB / Analytics Store
```

For a system-design interview, mentioning this will significantly improve your answer.

---

# Search is missing

You list:

> search videos

but your architecture doesn't have a search system.

Don't try to do:

```sql
SELECT *
FROM videos
WHERE title LIKE '%football%';
```

at YouTube scale.

Introduce something like:

```text
                 Video Service
                      │
                      │ video created/updated
                      ▼
                    Queue
                      │
                      ▼
                Search Index
                      │
                      ▼
                 Elasticsearch
```

Then:

```text
Client
  │
  │ search "football"
  ▼
Search Service
  │
  ▼
Search Index
```

This is a separate concern from your primary DB.

---

# One more important thing: video states

Your video shouldn't immediately be considered playable after upload.

Use a state machine:

```text
UPLOADING
    │
    ▼
UPLOADED
    │
    ▼
PROCESSING
    │
    ├──── failure ───► FAILED
    │
    ▼
READY
```

Then:

```text
GET /videos/123
```

could return:

```json
{
  "videoId": "123",
  "status": "PROCESSING"
}
```

Once transcoding finishes:

```json
{
  "videoId": "123",
  "status": "READY",
  "manifest": "..."
}
```

This makes your design much cleaner.

---

# Your API design can also improve

Instead of:

```text
POST /upload ← videoID
```

I'd use:

```text
POST /videos
```

Creates metadata and returns:

```json
{
  "videoId": "123",
  "uploadId": "...",
  "parts": [...]
}
```

Then:

```text
POST /videos/{videoId}/complete
```

after multipart upload.

Playback:

```text
GET /videos/{videoId}
```

Like:

```text
POST /videos/{videoId}/like
DELETE /videos/{videoId}/like
```

Comment:

```text
POST /videos/{videoId}/comments
GET  /videos/{videoId}/comments
```

Search:

```text
GET /search?q=...
```

This is cleaner than:

```text
POST /like {type: +1/-1}
```

because HTTP methods can naturally represent the operation.

---

# What I'd draw in an interview

I'd evolve your diagram into roughly:

```text
                         ┌──────────────┐
                         │     CDN      │
                         └──────┬───────┘
                                │
                                ▼
                              S3
                                ▲
                                │
                       ┌────────┴────────┐
                       │ Video Processor │
                       │   Workers       │
                       └────────▲────────┘
                                │
                              Queue
                                ▲
                                │ S3 Event
                                │
Client ──► API Gateway ──► Video Service
  │                         │       │
  │                         │       ▼
  │                         │       DB
  │                         │
  │                         └──► Search Index
  │
  └──────────────────────────────► S3
             multipart upload
```

Then separately:

```text
Client
  │
  ├── upload ───────────────► S3
  │
  ├── watch ────────────────► CDN ──► S3
  │
  ├── like/comment ────────► API ──► DB
  │
  └── search ───────────────► Search Service ──► Search Index
```

## Overall assessment

**Your architecture is fundamentally on the right track.** The biggest things you need to add/fix are:

1. **S3 multipart + presigned URLs** for large/resumable uploads.
2. **S3 event → queue → transcoding workers**.
3. **HLS/DASH adaptive bitrate streaming**, not just "different formats."
4. **Manifest/prefix in DB, not every segment URL.**
5. **CDN → S3** explicitly.
6. **Search index** for video search.
7. **Video processing state machine**.
8. **Asynchronous view-count aggregation** to avoid hot rows.
9. **Idempotency/retries** around queue processing.
10. **Separate read-heavy paths from transactional DB operations.**

If this is for a **system-design interview**, I'd focus next on **scale calculations + bottlenecks**. That's where you can take this from a decent design to a strong one: e.g. *100M daily users, 10M uploads/day, average video size 500 MB, 1M concurrent viewers — how much S3 storage, network bandwidth, CDN traffic, DB QPS, and transcoding capacity do we need?*
