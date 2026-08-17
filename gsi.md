
A **Global Secondary Index (GSI)** is an additional index in DynamoDB that lets you **query the same data using a different partition key and/or sort key** than the table's primary key.

The easiest way to understand it is through an example.

### Without a GSI

Suppose your `Orders` table has:

```text
Primary Key:
Partition Key = user_id
Sort Key      = order_id
```

Data:

| user_id | order_id | product  | status  |
| ------- | -------- | -------- | ------- |
| U1      | O1       | Laptop   | SHIPPED |
| U1      | O2       | Mouse    | PENDING |
| U2      | O3       | Keyboard | SHIPPED |
| U3      | O4       | Monitor  | PENDING |

You can efficiently ask:

> "Give me all orders of user U1."

Because `user_id` is the partition key.

But suppose you want:

> "Give me all PENDING orders."

`status` is **not** part of the primary key, so you cannot efficiently query the table by `status`. You would need a scan, which is expensive at scale.

---

## Add a Global Secondary Index

Create:

```text
GSI:
Partition Key = status
Sort Key      = order_id
```

Now DynamoDB maintains another index:

```text
GSI
-----------------------------
status      order_id
PENDING     O2
PENDING     O4
SHIPPED     O1
SHIPPED     O3
```

Now you can efficiently query:

```text
status = "PENDING"
```

and get:

```text
O2
O4
```

without scanning the entire table.

---

## Why is it called "Global"?

Because the index isn't restricted to a single partition key value of the base table.

Your original table is organized around:

```text
user_id
```

while the GSI is independently organized around:

```text
status
```

Conceptually:

```text
                 Orders Table
              PK: user_id
              SK: order_id
                    │
                    │
                    ▼
          ┌─────────────────────┐
          │ Global Secondary    │
          │ Index               │
          │                     │
          │ PK: status          │
          │ SK: order_id        │
          └─────────────────────┘
```

The GSI is essentially another way of organizing/indexing the same underlying data.

---

## GSI vs Primary Key

Suppose:

```text
Table Primary Key:
PK = user_id
SK = order_id
```

You can query:

```text
user_id = U1
```

But you can't efficiently query:

```text
status = PENDING
```

unless `status` is indexed.

So you create:

```text
GSI:
PK = status
SK = order_id
```

Now you have **two access patterns**:

```text
                    Orders
                      │
          ┌───────────┴───────────┐
          ↓                       ↓
   Primary Key                 GSI
   user_id                     status
      ↓                           ↓
"orders for U1"          "pending orders"
```

---

## Very important: GSI is designed around access patterns

This is one of the most important DynamoDB concepts for system design.

Don't think:

> "I'll create indexes for all columns."

Instead think:

> **"What queries does my application need to perform?"**

For example, an e-commerce system might need:

```text
1. Get orders for a user
   → PK = user_id

2. Get all orders with a particular status
   → GSI PK = status

3. Get orders for a user within a time range
   → PK = user_id
   → SK = created_at

4. Find orders by seller
   → another GSI
```

DynamoDB schema design is therefore heavily **access-pattern driven**.

---

### One more important difference

**GSI does not have to use the same keys as the base table.**

For example:

```text
Base table:
PK = user_id
SK = order_id

GSI:
PK = seller_id
SK = created_at
```

This allows a completely different query pattern:

```text
seller_id = S123
AND created_at > 2026-08-01
```

---

### GSI vs LSI

You'll encounter **LSI (Local Secondary Index)** too.

|                 | GSI                                          | LSI                                        |
| --------------- | -------------------------------------------- | ------------------------------------------ |
| Partition key   | Can be different                             | Must be same as table                      |
| Sort key        | Different                                    | Different                                  |
| Created         | Can be created after table creation          | Must be created when table is created      |
| Partition limit | Not tied to same 10 GB item-collection limit | Subject to 10 GB item collection limit     |
| Common use      | Completely different access pattern          | Alternative ordering within same partition |

**The one sentence to remember:**

> **A GSI gives you another way to query your DynamoDB table by providing a different partition key (and optionally sort key).**

For system-design interviews, the next concept you should learn is **GSI partition-key design and hot partitions**, because that's where the real scalability implications start.
