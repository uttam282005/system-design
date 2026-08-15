
## Concurrency handling in the Task Management System

Concurrency means **multiple users/requests can modify the same task at the same time**.

This is a very common LLD interview follow-up.

Suppose Task `T1` is currently:

```text
Status = TODO
Priority = HIGH
Version = 5
```

Two users open the same task:

```text
              Task T1
             Version 5
             /       \
            /         \
       User A        User B
       version 5     version 5
```

User A changes status:

```text
TODO → IN_PROGRESS
```

User B changes priority:

```text
HIGH → LOW
```

Both are working with **version 5**.

---

# 1. The naive approach — lost update

Imagine both do:

```text
User A:
read task
modify status
save

User B:
read task
modify priority
save
```

Suppose A saves first:

```text
Task:
status = IN_PROGRESS
priority = HIGH
```

Then B saves its older copy:

```text
Task:
status = TODO
priority = LOW
```

A's update is gone.

This is called a **lost update**.

---

# 2. First solution: pessimistic locking

We could lock the database row:

```sql
SELECT *
FROM tasks
WHERE id = 1
FOR UPDATE;
```

Conceptually:

```text
User A
   |
   | lock Task 1
   ↓
Task 1 🔒
   |
   | modify
   |
   | commit
   ↓
unlock
   |
   ↓
User B can now modify
```

### Advantage

Very strong consistency.

### Problem

If many users are modifying tasks:

```text
Task 1 🔒
Task 1 🔒
Task 1 🔒
Task 1 🔒
```

requests wait for each other.

It can reduce concurrency and introduce deadlocks if multiple rows are locked in inconsistent order.

For this task-management system, **I wouldn't start with pessimistic locking**.

---

# 3. Better approach: Optimistic concurrency control

For a task management system, I'd normally use **optimistic locking**.

Add a version field:

```cpp
class Task {
private:
    int id;

    TaskStatus status;
    Priority priority;

    uint64_t version;
};
```

Initially:

```text
Task 1
version = 5
```

---

# 4. User A reads

```text
User A
   |
   ↓
GET Task 1

status = TODO
version = 5
```

User B does the same:

```text
User B
   |
   ↓
GET Task 1

status = TODO
version = 5
```

Both have:

```text
version = 5
```

---

# 5. User A updates

A sends:

```text
taskId = 1
newStatus = IN_PROGRESS
expectedVersion = 5
```

Database query:

```sql
UPDATE tasks
SET status = 'IN_PROGRESS',
    version = version + 1
WHERE id = 1
  AND version = 5;
```

The update succeeds.

Now:

```text
Task 1
status  = IN_PROGRESS
version = 6
```

---

# 6. User B tries to update

B still thinks:

```text
version = 5
```

So it sends:

```sql
UPDATE tasks
SET priority = 'LOW',
    version = version + 1
WHERE id = 1
  AND version = 5;
```

But the database currently has:

```text
version = 6
```

Therefore:

```text
0 rows updated
```

We know that someone modified the task after B read it.

So return:

```text
409 Conflict
```

or a domain exception:

```cpp
throw ConcurrentModification{};
```

---

# 7. Why this works

The important part is:

```sql
WHERE version = expectedVersion
```

The update is effectively:

```text
"If nobody changed this task since I read it,
apply my change."
```

This gives us an atomic check-and-update.

---

# 8. What should the user do after conflict?

Usually:

```text
Client
  |
  | update(version=5)
  ↓
Server
  |
  | version is actually 6
  ↓
409 Conflict
  |
  ↓
Client reloads task
  |
  ↓
User decides what to do
```

For example:

```text
Your version:
status = TODO
priority = LOW

Current version:
status = IN_PROGRESS
priority = HIGH
```

The UI can ask the user to reload/reapply their change.

---

# 9. Where does version live?

Database:

```text
tasks
--------------------------------
id
title
description
status
priority
assignee_id
version
created_at
updated_at
```

Example:

```text
id    status         version
1     TODO              5
2     DONE              9
3     IN_PROGRESS       2
```

Every successful update increments the version.

---

# 10. Repository responsibility

This is an important LLD point.

Don't put SQL concurrency logic inside `Task`.

`Task` should know:

```text
business rules
```

Repository should know:

```text
database concurrency
```

So:

```text
TaskService
     |
     ↓
Task
     |
     ↓
ITaskRepository
     |
     ↓
PostgreSQL
```

The repository can expose something like:

```cpp
class ITaskRepository {
public:
    virtual std::shared_ptr<Task>
    findById(int id) = 0;

    virtual bool update(
        const Task& task,
        uint64_t expectedVersion
    ) = 0;
};
```

Then:

```cpp
bool PostgresTaskRepository::update(
    const Task& task,
    uint64_t expectedVersion
) {
    // UPDATE ... WHERE id=? AND version=?
}
```

---

# 11. Important distinction: thread safety vs concurrency control

These are **not the same thing**.

### Thread safety

Protecting memory inside one process.

Example:

```cpp
std::mutex mutex;
```

### Concurrency control

Handling multiple requests modifying shared persistent data.

Example:

```text
Optimistic locking
Pessimistic locking
Database transactions
```

In a real backend:

```text
Server 1 ──┐
Server 2 ──┼──→ PostgreSQL
Server 3 ──┘
```

A C++ mutex in Server 1 **cannot protect data being modified by Server 2**.

That's why database-level concurrency control matters.

---

# 12. What about two different fields?

Suppose:

```text
User A:
change status

User B:
change priority
```

Could we allow both?

Potentially yes, but it depends on your consistency model.

With simple entity-level versioning:

```text
A updates → version 5 → 6
B sees conflict
```

Even though they modified different fields.

That's actually a **safe and simple choice**.

Don't try to implement field-level merging unless the requirements demand it.

For a 45-minute interview, say:

> "I'll use entity-level optimistic locking. If the product requires automatic merging of independent field updates, we could introduce finer-grained conflict resolution later."

Excellent answer.

---

# 13. What about assigning a task?

Same mechanism.

Initial:

```text
assignee = Alice
version = 10
```

Two managers:

```text
Manager A → assign Bob
Manager B → assign Charlie
```

A succeeds:

```text
assignee = Bob
version = 11
```

B's update:

```sql
WHERE id = ?
AND version = 10
```

fails.

Therefore Charlie doesn't silently overwrite Bob.

---

# 14. Transactions

Optimistic locking should happen inside the database operation/transaction where appropriate.

For example:

```text
BEGIN

UPDATE task
WHERE id = 1
AND version = 5

INSERT audit_log ...

COMMIT
```

This ensures related changes are consistent.

---

# 15. Concurrency + events

Now combine this with the production architecture we discussed.

Suppose A successfully updates a task.

We need:

```text
Task update
+
Audit event
+
Notification event
```

Don't do:

```text
UPDATE task
COMMIT

send Kafka event
```

because Kafka could fail after the DB update.

Use the **Transactional Outbox**:

```text
BEGIN
   |
   ├── UPDATE task
   |
   ├── INSERT outbox_event
   |
COMMIT
```

Then:

```text
Outbox
   ↓
Publisher
   ↓
Kafka
   ↓
Notification / Audit / Analytics
```

So the complete production approach becomes:

```text
              Request
                 |
                 ↓
            TaskService
                 |
                 ↓
          Task business rules
                 |
                 ↓
       Optimistic locking
                 |
                 ↓
             PostgreSQL
                 |
          ┌──────┴──────┐
          ↓             ↓
        Task          Outbox
                        |
                        ↓
                      Kafka
```

---

# 16. What to say in the interview

If the interviewer asks:

> **"How will you handle concurrent updates to a task?"**

Give this answer:

> "I'll use optimistic locking. Each task has a version number. When a client reads a task, it gets the current version. When updating, the repository performs an atomic update with `WHERE task_id = ? AND version = ?`, and increments the version on success. If zero rows are affected, another request modified the task, so I return a concurrent modification or 409 conflict. This avoids global locks and works across multiple server instances."

That's a **very strong 30–45 second answer**.

If they ask:

> **"Why not mutex?"**

Say:

> "A mutex only protects memory within one process. In a distributed deployment, multiple application instances can modify the same database row, so concurrency must ultimately be coordinated at the persistence layer."

And if they ask:

> **"Why optimistic instead of pessimistic?"**

Say:

> "Task updates are generally short and contention is expected to be relatively low, so optimistic locking gives better concurrency without holding database locks during the whole request. If contention becomes high or an operation requires strict serialization, pessimistic locking could be appropriate."
