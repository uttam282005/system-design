### **1. System Requirements & Goals** [[01:24](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=84)]

An interview should begin by defining the scope. The design focuses on a simplified microblogging service.

* **Functional Requirements** [[01:46](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=106)]
* Users can create an account, log in, and manage profiles.
* Users can create, edit, delete, and like tweets.
* Users can follow/unfollow other users.
* Users can view a timeline (feed) of tweets from people they follow.
* Users can reply to tweets and retweet.
* Users can search for tweets, profiles, or trending hashtags.


* **Non-Functional Requirements** [[02:11](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=131)]
* **Scale:** Support hundreds of millions of daily active users (DAUs).
* **Availability:** High availability with 99.99% uptime.
* **Latency:** Ultra-low latency for loading timelines (fast read path).


* **Pro-Tip:** Do not spend more than 5 minutes on this section during an actual interview. [[03:13](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=193)]

---

### **2. High-Level Architecture & Routing** [[03:30](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=210)]

The basic request lifecycle operates from left to right:

* **Clients:** Web and mobile applications (iOS/Android). [[03:41](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=221)]
* **Load Balancer (Layer 7):** Distributes incoming traffic across API gateways. [[03:51](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=231)]
* *Routing Algorithm:* **Round Robin** is selected because backend servers are stateless and persistent connections are not required. [[05:01](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=301)]
* *OSI Layer:* Operating at Layer 7 (Application layer) allows content-based routing (e.g., routing based on URLs or headers), which helps with feature rollouts. [[05:11](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=311)]


* **API Gateway:** Routes the traffic to specialized microservices. [[05:23](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=323)]

---

### **3. Microservices & Data Storage** [[06:06](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=366)]

#### **A. Tweet CRUD Service** [[06:17](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=377)]

Handles creation, deletion, modifications, likes, and retweets. It is optimized for high write and read throughput.

* **Database (MongoDB):** A NoSQL document store is chosen to save tweets as JSON/key-value pairs (storing metrics, mentions, and text). No SQL joins are needed because a single document can represent the whole tweet. [[07:44](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=464)]
* **Media Storage (Amazon S3):** Video and image assets are separated from metadata and stored in blob object storage. [[08:34](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=514)]
* **Content Delivery Network (CDN):** Static assets and popular tweets are cached closer to the geographic location of users to minimize read latency. [[10:13](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=613)]

#### **B. Reply Service** [[06:42](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=402)]

Separated from the Tweet service so it can scale independently, particularly for viral tweets that generate thousands of rapid replies. [[10:45](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=645)]

* **Storage Strategy:** Replies are stored in their own database, indexed by `tweet_id`. This allows the system to fetch the main tweet structure instantly without loading heavy reply structures until the user explicitly scrolls down. [[11:15](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=675)]

#### **C. Search Service** [[06:55](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=415)]

* **Elasticsearch:** A distributed full-text search engine provides text-based indexing on tweet contents, usernames, and hashtags. [[13:10](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=790)]
* **Change Data Capture (CDC):** As data gets written or modified in the main NoSQL database, updates automatically trigger updates to the Elasticsearch cluster. [[13:45](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=825)]

#### **D. Profile & Social Graph Service** [[07:04](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=424)]

* **Relational Database (SQL):** Standard user metadata (email, billing, hashed password, bio) fits highly structured SQL relational rows, ensuring strict ACID consistency. [[16:53](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1013)]
* **Graph Database (Neo4j/GraphDB):** Follower networks and social graph connections are kept in a graph database, which makes social connection queries much faster and allows for future recommendation features. [[17:24](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1044)]
* **Auth Service:** A dedicated component handles user authorization/authentication for tightened security and flexible integration. [[17:45](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1065)]

---

### **4. The Core Engineering Challenge: Timeline Generation** [[14:04](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=844)]

Constructing a user’s feed dynamically is the most vital part of a social media system design.

| Strategy | Mechanism | Pros | Cons |
| --- | --- | --- | --- |
| **Fan-out on Read** [[14:34](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=874)] | When a user opens their home page, pull followings, fetch all tweets, sort by time, and serve. | Cheap on system writes. | Extremely slow and read-intensive at scale. |
| **Fan-out on Write** [[14:55](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=895)] | When a tweet is created, push it onto a **Message Queue**. Background workers fetch follower lists and immediately update/prepend the target tweet into every follower's **Timeline Cache** (Redis/Memcached). | Read path is lightning fast since data is prepared beforehand. | Write-intensive; breaks down when a celebrity posts. |

* **The Hybrid Solution:** [[16:10](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=970)] For normal users, the system uses **Fan-out on Write**. However, for "Mega-Influencers" (users with millions of followers), the system switches to **Fan-out on Read** for their specific posts. When a follower loads their feed, the system pulls regular posts from their pre-built cache and merges them with the celebrity's recent tweets.

---

### **5. Security, Monitoring, and Reliability** [[18:22](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1102)]

#### **A. Security Measures** [[18:29](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1109)]

* **Rate Limiting:** Implemented at the API Gateway level (IP rate limiting) to avoid DDOS attacks and generic bot floods, and specifically on write actions (tweeting/replying). [[09:31](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=571)], [[19:15](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1155)]
* **Data Encryption:** TLS/HTTPS protects transit lines, while modern database features handle encryption at rest. [[18:48](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1128)]
* **Input Validation:** Sanitize fields on both client and server sides to protect against cross-site scripting (XSS) and code injection. [[19:35](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1175)]

#### **B. Monitoring & Logging** [[19:49](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1189)]

* **Metrics & Dashboards:** **Prometheus** tracks raw time-series performance data, while **Grafana** provides visualization dashboards for health tracking. [[20:07](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1207)]
* **Log Management:** An **ELK Stack** (Elasticsearch, Logstash, Kibana) centralizes and indexes microservice logs to assist in rapid debugging. [[20:28](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1228)]
* **On-Call Alerts:** Systems link to **PagerDuty** or Alertmanager to send high-priority notifications to engineers when failures occur. [[21:01](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1261)]

#### **C. Testing Standards** [[21:23](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1283)]

* **Load Testing:** Simulates high traffic loads on core services before releasing new features. [[21:23](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1283)]
* **CI/CD Pipeline:** Uses tools like Jenkins or GitHub Actions to automatically run unit and integration tests on every pull request. [[21:45](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1305)]
* **Disaster Recovery:** Periodic automated validation tests verify backup reliability and cluster recovery procedures. [[22:01](http://www.youtube.com/watch?v=Nfa-uUHuFHg&t=1321)]
