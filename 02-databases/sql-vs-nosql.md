# SQL vs. NoSQL Databases: Quick Revision

Choosing the right database is a critical system design decision. Here’s a high-level comparison to help you decide.

---

## Core Comparison: SQL vs. NoSQL

| Feature         | SQL (Relational)                                     | NoSQL (Non-Relational)                               |
| :-------------- | :--------------------------------------------------- | :--------------------------------------------------- |
| **Data Model**  | Tables with rows and columns (structured data).      | Varies: Documents, Key-Value, Column-Family, Graph.  |
| **Schema**      | **Fixed & Predefined:** Structure must be defined upfront. | **Flexible & Dynamic:** Fields can be added on the fly. |
| **Consistency** | **ACID** properties (Atomic, Consistent, Isolated, Durable). Guarantees data integrity. | **BASE** (Basically Available, Soft state, Eventually consistent). Prioritizes availability. |
| **Scaling**     | **Vertical Scaling** (scaling up by adding more power like CPU/RAM to one server). | **Horizontal Scaling** (scaling out by adding more servers to a cluster). |
| **Queries**     | Powerful for `JOIN`s, complex queries, and aggregations. | Queries are often simpler, with limited `JOIN` capabilities. |
| **Examples**    | MySQL, PostgreSQL, Oracle, SQL Server               | MongoDB, Redis, Cassandra, Neo4j                     |

---

## Types of NoSQL Databases

NoSQL is a broad category. Here are the main types:

1.  **Document Stores**
    *   **What:** Stores data in flexible, JSON-like documents.
    *   **Example:** MongoDB
    *   **Use Case:** Content management, user profiles.
    ```json
    {
      "userId": 123,
      "username": "alex",
      "posts": [
        {"postId": 1, "title": "My First Post"},
        {"postId": 2, "title": "My Second Post"}
      ]
    }
    ```

2.  **Key-Value Stores**
    *   **What:** The simplest model. Stores data as a key that maps to a value.
    *   **Example:** Redis, DynamoDB
    *   **Use Case:** Caching, session management.
    ```
    "user:123:session" -> "ewo...session_data...fQ=="
    ```

3.  **Column-Family Stores**
    *   **What:** Stores data in columns rather than rows. Optimized for fast reads/writes over a large number of columns.
    *   **Example:** Apache Cassandra
    *   **Use Case:** Big data analytics, logging, time-series data.

4.  **Graph Databases**
    *   **What:** Models data as nodes and edges, focusing on the relationships between data points.
    *   **Example:** Neo4j
    *   **Use Case:** Social networks, recommendation engines, fraud detection.
    ```mermaid
    graph TD
        User_A[("User A")] -- "FRIENDS" --> User_B[("User B")]
        User_A -- "LIKES" --> Post_1[("Post 1")]
        User_B -- "LIKES" --> Post_1
    ```

---

## When to Use Which?

| Use SQL When...                                         | Use NoSQL When...                                       |
| :------------------------------------------------------ | :------------------------------------------------------ |
| ✅ **Data integrity is critical** (ACID compliance).      | ✅ **You need high availability and scalability.**        |
| ✅ Your data is **structured** with a fixed schema.       | ✅ Your data is **unstructured or semi-structured.**    |
| ✅ You need to perform **complex queries and JOINs**.     | ✅ You need a **flexible schema** that can evolve quickly.|
| **Examples:**                                           | **Examples:**                                           |
| <li>Financial Systems (banking, trading)</li>           | <li>Social Media Feeds (posts, likes, comments)</li>    |
| <li>E-commerce Orders & Payments</li>                   | <li>Real-time data (IoT sensor data, user locations)</li> |
| <li>Data Warehousing & Analytics</li>                   | <li>Content Management Systems & Catalogs</li>            |
