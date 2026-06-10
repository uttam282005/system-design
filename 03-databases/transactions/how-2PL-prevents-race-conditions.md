### Initial Values

```text
A = 100
B = 100
```

### Transactions

**T1**

```text
lock-X(A)
A = A + 10
unlock(A)

lock-X(B)
B = B + 10
unlock(B)
```

**T2**

```text
lock-X(B)
B = 2 * B
unlock(B)

lock-X(A)
A = 2 * A
unlock(A)
```

Notice that neither transaction follows 2PL because each releases a lock and then acquires another one later.

---

### Interleaved Execution

```text
T1: lock(A)
T1: A = 110
T1: unlock(A)

T2: lock(B)
T2: B = 200
T2: unlock(B)

T1: lock(B)
T1: B = 210
T1: unlock(B)

T2: lock(A)
T2: A = 220
T2: unlock(A)
```

Final state:

```text
A = 220
B = 210
```

---

### Check Serial Orders

#### If T1 ran completely before T2

```text
After T1:
A = 110
B = 110

After T2:
A = 220
B = 220
```

Result:

```text
A = 220
B = 220
```

#### If T2 ran completely before T1

```text
After T2:
A = 200
B = 200

After T1:
A = 210
B = 210
```

Result:

```text
A = 210
B = 210
```

---

### Problem

The interleaved execution produced:

```text
A = 220
B = 210
```

which matches **neither** serial order.

Therefore, the schedule is **not serializable**.

---

### What 2PL Would Do

Under 2PL, T1 cannot do:

```text
lock(A)
unlock(A)
lock(B)   ❌
```

Once T1 releases `A`, it enters the **shrinking phase** and is forbidden from acquiring `B`.

So T1 must instead do:

```text
lock(A)
lock(B)
...
unlock(A)
unlock(B)
```

or

```text
lock(B)
lock(A)
...
unlock(B)
unlock(A)
```

Because all locks are acquired before any are released, the resulting schedules are guaranteed to be conflict-serializable.

A useful way to think about it is:

> 2PL forces every transaction to "declare its territory" (acquire all needed locks) before it starts giving any territory back. This prevents transactions from weaving through each other in ways that create impossible serial orders.
