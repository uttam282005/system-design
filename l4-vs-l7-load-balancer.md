
The key difference is **which layer of the network stack the load balancer understands**.

|                    | **L4 Load Balancer**                          | **L7 Load Balancer**                            |
| ------------------ | --------------------------------------------- | ----------------------------------------------- |
| Layer              | Transport layer                               | Application layer                               |
| Understands        | TCP/UDP, IP, ports                            | HTTP/HTTPS, URLs, headers, cookies              |
| Routing based on   | IP + port                                     | URL, hostname, headers, cookies, etc.           |
| Payload inspection | ❌ No                                          | ✅ Yes                                           |
| Performance        | Generally faster                              | Generally slightly slower                       |
| TLS termination    | Usually possible, depending on implementation | Common                                          |
| Typical use        | TCP services, databases, gRPC, game servers   | Web/API applications                            |
| Example routing    | `10.0.0.5:443 → server A`                     | `/api → API servers`, `/images → image servers` |

### L4 example

Suppose you have:

```text
                ┌── Server A
Client ──TCP──> L4 LB
                ├── Server B
                └── Server C
```

The L4 LB sees something like:

```text
Source IP: 192.168.1.10
Destination IP: 10.0.0.100
Destination Port: 443
Protocol: TCP
```

It **doesn't care what the HTTP request contains**.

So these two requests look essentially the same to an L4 LB:

```http
GET /users
```

and

```http
GET /payments
```

It can distribute connections based on things like:

```text
source IP
destination IP
source port
destination port
TCP connection
```

---

### L7 example

An L7 load balancer understands HTTP.

```text
                    ┌── API servers
                    │
Client → L7 LB ─────┼── Image servers
                    │
                    └── Web servers
```

It can inspect:

```http
GET /api/users
Host: example.com
Authorization: ...
Cookie: ...
```

and make decisions such as:

```text
/api/*       → API cluster
/images/*    → Image cluster
/admin/*     → Admin cluster
example.com  → Cluster A
api.foo.com  → Cluster B
```

That's something an ordinary L4 LB cannot do because it doesn't understand the HTTP request.

---

## The most important conceptual difference

Think of it this way:

**L4:**

> "I have a TCP connection. Which server should receive it?"

**L7:**

> "I have an HTTP request. Where should this particular request go?"

---

## Connection vs request

This is particularly important in system design interviews.

Suppose you have:

```text
Client
  ↓
Load Balancer
  ↓
Server A
```

With **L4**, the LB can establish/forward a TCP connection to Server A. Subsequent packets belonging to that connection generally remain associated with that backend.

With **L7**, the LB can terminate the client connection, inspect HTTP requests, and create its own backend connections:

```text
Client
  │
  │ HTTP
  ▼
L7 LB
  │
  ├── HTTP → Server A
  ├── HTTP → Server B
  └── HTTP → Server C
```

Therefore, L7 can make **per-request routing decisions**.

---

## When would you choose each?

### Use L4 when:

* You need very high throughput / low overhead.
* You're balancing arbitrary TCP/UDP traffic.
* The application protocol isn't HTTP.
* You want simple connection-level load balancing.
* You're dealing with services such as TCP databases, custom protocols, etc.

### Use L7 when:

* You're building HTTP/HTTPS APIs.
* You need routing based on URL/path.
* You need hostname-based routing.
* You need cookie/session-based routing.
* You need HTTP header manipulation.
* You want application-aware routing.
* You want features like WAF integration, HTTP caching, redirects, etc.

---

### Interview example

If asked:

> **"We have `/users`, `/orders`, and `/payments` APIs and want to route each to different backend clusters. L4 or L7?"**

**L7**, because routing depends on the HTTP request path.

If asked:

> **"We have a custom TCP protocol running on port 9000 and need to distribute connections across 100 servers."**

**L4**, because the load balancer doesn't need to understand the application protocol.

### One subtle point

**L4 ≠ necessarily "no TLS" and L7 ≠ necessarily "HTTP only."**

The distinction is about **how deeply the load balancer understands the traffic**. L7 commonly means HTTP-aware load balancing, while L4 works primarily with transport-level information such as TCP/UDP connections.

For system design, remember:

> **L4 = connection-level load balancing**
> **L7 = application/request-level load balancing**
