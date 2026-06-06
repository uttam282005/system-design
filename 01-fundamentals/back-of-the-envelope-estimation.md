# Back-of-the-Envelope Estimation: Quick Revision

---

## 1. Load Estimation

*   **Goal:** Estimate read and write requests per second.
*   **Start with:** Daily Active Users (DAU).

**Example: Twitter**

*   **DAU:** 100 million
*   **User Writes:** 10 tweets/day
*   **User Reads:** 1000 tweets/day

**Calculations:**

*   **Total Writes/day:**
    100 million users * 10 tweets/user = **1 billion writes/day**

*   **Total Reads/day:**
    100 million users * 1000 tweets/user = **100 billion reads/day**

---

## 2. Storage Estimation

*   **Goal:** Estimate the total data storage needed.
*   **Consider:** Different data types and their sizes (e.g., text, images).

**Example: Twitter Storage**

*   **Assumptions:**
    *   1 tweet (text only) ≈ 500 bytes
    *   1 photo = 2 MB
    *   10% of tweets have photos.

**Calculations:**

*   **Total tweets/day:** 1 billion
*   **Tweets with photos/day:** 10% of 1 billion = 100 million

*   **Daily Storage Required:**
    *   `(Size of one tweet * Total tweets)` + `(Size of photo * Total tweets with photo)`
    *   `(500 bytes * 1 billion)` + `(2 MB * 100 million)`
    *   This is roughly **1 PB/day**.

---

## 3. Resource Estimation

*   **Goal:** Estimate the number of servers/CPUs required.

**Example: Handling Web Requests**

*   **Assumptions:**
    *   **Requests per second:** 10,000
    *   **CPU time per request:** 10 ms
    *   **CPU core capacity:** 1000 ms/sec
    *   **Server spec:** 4 cores per server

**Calculations:**

1.  **Total CPU Time Needed per Second:**
    *   10,000 req/sec * 10 ms/req = **100,000 ms/sec**

2.  **Total Cores Required:**
    *   100,000 ms/sec / 1000 ms/core = **100 cores**

3.  **Total Servers Required:**
    *   100 cores / 4 cores/server = **25 servers**

**System Diagram:**

```
      Incoming Requests
(10,000 requests/second)
             |
             v
      +--------------+
      | Load Balancer|
      +--------------+
      /   /   |   \   \
     /   /    |    \   \
    v   v     v     v   v
  +---+ +---+ +---+ +---+ +---+
  | S | | S | | S | | S | | S | ... (25 Servers Total)
  +---+ +---+ +---+ +---+ +---+
```

