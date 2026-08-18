For an **LLD interview**, “Design a Logger” is a very common problem. The interviewer usually wants to see whether you can go beyond a simple `log()` function and design for **levels, multiple outputs, extensibility, formatting, configuration, and concurrency**.

## 1. First clarify requirements

Before coding, ask:

### Functional

* Support log levels: `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`
* Log a message with timestamp and metadata.
* Configure the minimum log level.
* Support multiple destinations:

  * Console
  * File
  * Potentially DB/network later
* Support different formats.
* Logger should be thread-safe.

### Non-functional

* Easy to add new log destinations.
* Easy to add new formatting strategies.
* Low overhead.
* Shouldn't require modifying `Logger` when adding a new sink.

I'd explicitly state:

> "I'll design this around interfaces so logging destinations and formatting strategies can be extended without changing the core logger."

---

# 2. Core abstractions

A clean design is:

```text
                 +----------------+
                 |     Logger     |
                 +----------------+
                         |
             +-----------+-----------+
             |           |           |
             v           v           v
        ConsoleSink   FileSink   NetworkSink
             |
             +------------------+
                                |
                         LogFormatter
                         /          \
                        v            v
                 TextFormatter   JsonFormatter
```

The important abstractions are:

### `LogLevel`

```cpp
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};
```

### `LogMessage`

Don't pass around raw strings everywhere.

```cpp
struct LogMessage {
    LogLevel level;
    std::string message;
    std::string loggerName;
    std::string timestamp;
    std::thread::id threadId;
};
```

This gives us a structured representation of a log event.

---

# 3. Logger

The `Logger` is responsible for:

* checking log level
* creating `LogMessage`
* formatting
* sending it to sinks

```cpp
class Logger {
private:
    LogLevel minLevel;

    std::vector<std::unique_ptr<ILogSink>> sinks;
    std::unique_ptr<ILogFormatter> formatter;

    std::mutex mutex;

public:
    void log(LogLevel level, const std::string& message);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void fatal(const std::string& msg);
};
```

But I'd make the interfaces explicit.

---

# 4. Sink abstraction

This is essentially the **Strategy / Polymorphism** part.

```cpp
class ILogSink {
public:
    virtual ~ILogSink() = default;

    virtual void write(const std::string& formattedMessage) = 0;
};
```

Implementations:

```cpp
class ConsoleSink : public ILogSink {
public:
    void write(const std::string& message) override {
        std::cout << message << std::endl;
    }
};
```

```cpp
class FileSink : public ILogSink {
private:
    std::ofstream file;

public:
    explicit FileSink(const std::string& filename)
        : file(filename, std::ios::app) {}

    void write(const std::string& message) override {
        file << message << '\n';
    }
};
```

Later:

```cpp
class NetworkSink : public ILogSink {
    // send to remote logging service
};
```

The `Logger` doesn't care where the message goes.

That's an important design principle:

> **Program to interfaces, not implementations.**

---

# 5. Formatter abstraction

Separate formatting from logging.

```cpp
class ILogFormatter {
public:
    virtual ~ILogFormatter() = default;

    virtual std::string format(const LogMessage& log) = 0;
};
```

For example:

```cpp
class TextFormatter : public ILogFormatter {
public:
    std::string format(const LogMessage& log) override {
        return "[" + log.timestamp + "] "
             + "[" + levelToString(log.level) + "] "
             + log.message;
    }
};
```

And:

```cpp
class JsonFormatter : public ILogFormatter {
public:
    std::string format(const LogMessage& log) override {
        // produce JSON
    }
};
```

Now we can have:

```text
Logger
  |
  +-- TextFormatter
  |
  +-- ConsoleSink
  +-- FileSink
```

or:

```text
Logger
  |
  +-- JsonFormatter
  |
  +-- FileSink
  +-- NetworkSink
```

without modifying Logger.

---

# 6. Logger flow

The important flow to explain in the interview:

```text
logger.error("Database connection failed")
              |
              v
       Check log level
              |
              v
       Create LogMessage
              |
              v
          Formatter
              |
              v
      "2026... [ERROR]..."
              |
       +------+------+
       |             |
       v             v
   Console         File
```

Implementation:

```cpp
void Logger::log(
    LogLevel level,
    const std::string& message
) {
    if (level < minLevel)
        return;

    LogMessage log{
        level,
        message,
        "Application",
        getCurrentTimestamp(),
        std::this_thread::get_id()
    };

    std::string formatted = formatter->format(log);

    std::lock_guard<std::mutex> lock(mutex);

    for (auto& sink : sinks) {
        sink->write(formatted);
    }
}
```

Then:

```cpp
void Logger::info(const std::string& msg) {
    log(LogLevel::INFO, msg);
}

void Logger::error(const std::string& msg) {
    log(LogLevel::ERROR, msg);
}
```

---

# 7. Important issue: concurrency

This is where you can differentiate yourself.

Imagine:

```text
Thread 1 ---> logger.info(...)
Thread 2 ---> logger.error(...)
Thread 3 ---> logger.warn(...)
```

If multiple threads write to the same file simultaneously, we can get interleaving/corruption.

At minimum:

```cpp
std::mutex mutex;
```

and:

```cpp
std::lock_guard<std::mutex> lock(mutex);
```

around sink writes.

But there's a better production design.

### Don't make application threads perform I/O

Instead:

```text
Application Threads
       |
       v
   Logger
       |
       v
   Thread-safe Queue
       |
       v
 Logging Worker
       |
       +----> Console
       |
       +----> File
       |
       +----> Network
```

The application thread only creates/enqueues the log.

The logging thread performs I/O.

This is an **asynchronous logger**.

---

# 8. Synchronous vs asynchronous

### Synchronous

```text
Application
    |
    v
 Logger
    |
    v
 File I/O
```

Advantages:

* simple
* log is immediately written

Disadvantages:

* disk/network I/O blocks application thread
* high latency under heavy logging

### Asynchronous

```text
Application
    |
    v
 Logger
    |
    v
 Queue
    |
    v
Worker Thread
    |
    v
File
```

Advantages:

* low application-thread latency
* handles high logging volume better

Disadvantages:

* more complicated
* logs may be lost if process crashes before queue is flushed
* queue needs backpressure policy

In an interview, **start synchronous** unless the interviewer asks for high throughput. Then evolve it to asynchronous.

---

# 9. Singleton?

You'll probably be tempted to do:

```cpp
class Logger {
public:
    static Logger& getInstance();
};
```

I would **not make Singleton the core design**.

Instead use dependency injection:

```cpp
class UserService {
private:
    Logger& logger;

public:
    UserService(Logger& logger)
        : logger(logger) {}
};
```

Why?

Singleton creates:

* hidden global state
* difficult testing
* tight coupling
* configuration problems

If the interviewer explicitly asks for a globally accessible logger, you can discuss Singleton as an option.

---

# 10. Better production-level design

I'd structure the final design as:

```text
                     Logger
                       |
              +--------+--------+
              |                 |
        LogLevelFilter       Formatter
                                |
                         +------+------+
                         |             |
                    TextFormatter  JsonFormatter
              |
              v
           Sink[]
        /     |      \
       /      |       \
 Console    File    Network
```

And for asynchronous logging:

```text
                  Logger
                    |
                    v
             BlockingQueue
                    |
                    v
              LoggingWorker
                    |
          +---------+---------+
          |         |         |
       Console    File     Network
```

---

# 11. Design patterns involved

This is worth explicitly mentioning during the interview.

### Strategy Pattern

Formatter:

```text
ILogFormatter
    |
    +-- TextFormatter
    +-- JsonFormatter
```

Different algorithms/strategies can be substituted.

### Strategy / Polymorphism

Sinks:

```text
ILogSink
   |
   +-- ConsoleSink
   +-- FileSink
   +-- NetworkSink
```

### Dependency Injection

Logger receives:

```cpp
vector<unique_ptr<ILogSink>>
ILogFormatter
```

rather than constructing them internally.

### Observer-like behavior

Conceptually, multiple sinks receive the same log event.

I wouldn't force the interviewer to call it Observer, though. **Composition + interfaces** is the important part.

---

# 12. Questions interviewer may push you on

Be prepared for these:

### "How would you add a database logger?"

Don't modify Logger.

```cpp
class DatabaseSink : public ILogSink {
    void write(...) override;
};
```

Add it to the sink list.

**Open/Closed Principle.**

---

### "How would you support JSON logs?"

Add:

```cpp
class JsonFormatter : public ILogFormatter
```

No Logger changes.

---

### "How do you make it thread-safe?"

Initially:

```cpp
mutex + lock_guard
```

For high throughput:

```text
multiple producers
       ↓
concurrent queue
       ↓
logging worker
```

---

### "What happens if the disk is full?"

`FileSink` should detect write failure.

Possible policies:

* report error to stderr
* retry
* rotate file
* drop logs
* fallback to another sink

Don't blindly call the logger again from inside the logger's error handling, or you can create recursive logging.

---

### "What if logging itself fails?"

Very important:

> Logging must not take down the application.

For example:

```text
Application
    |
    v
Logger
    |
    X File unavailable
    |
    v
Fallback stderr
```

---

### "How do you handle huge log files?"

Introduce:

```text
FileSink
   |
   +-- max file size
   +-- rotation policy
   +-- retention policy
```

Example:

```text
app.log
app.log.1
app.log.2
app.log.3
```

---

# 13. What I'd actually implement in an interview

Don't overengineer it.

Start with:

```text
LogLevel
LogMessage
ILogFormatter
TextFormatter
ILogSink
ConsoleSink
FileSink
Logger
```

Then implement:

```cpp
logger.info("Server started");
logger.error("Database unavailable");
```

Then interviewer asks:

> "What about concurrency?"

Add mutex.

Then:

> "What about high throughput?"

Explain asynchronous queue + worker.

Then:

> "What about JSON?"

Add formatter.

Then:

> "What about log rotation?"

Extend `FileSink`.

This demonstrates **incremental design**, which is much better than dumping a 500-line framework immediately.

### The key LLD lesson

For this problem, don't focus on making `Logger` itself clever.

Focus on **separating responsibilities**:

```text
Logger       → orchestration/filtering
LogMessage   → data
Formatter    → representation
Sink         → destination
Queue        → asynchronous buffering
Worker       → background I/O
```

That's the core design interviewers are looking for.
