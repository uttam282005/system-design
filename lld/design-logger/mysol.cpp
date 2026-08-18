/* functional Requirements-
 * 1. info, warn and error levels.
 * 2. json and plain text output format.
 * 3. console and file output destinations.
 */

/* non-funcitonal Requirements-
* 1. info levels should be extensible.
* 2. output destinations should be extensible.
* 3. output formats shoudl be extensible.
* 4. thread safe.
*/

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

// ============================================================
// Log Level
// ============================================================

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL
};

string toString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
    }

    return "UNKNOWN";
}


// ============================================================
// Log Message
// ============================================================

struct LogMessage {
    LogLevel level;
    string message;
    string timestamp;
    thread::id threadId;
};


// ============================================================
// Formatter
// ============================================================

class ILogFormatter {
public:
    virtual ~ILogFormatter() = default;

    virtual string format(const LogMessage& log) const = 0;
};


class TextFormatter : public ILogFormatter {
public:
    string format(const LogMessage& log) const override {
        ostringstream out;

        out << "[" << log.timestamp << "] "
            << "[" << toString(log.level) << "] "
            << "[Thread: " << log.threadId << "] "
            << log.message;

        return out.str();
    }
};


// ============================================================
// Sink
// ============================================================

class ILogSink {
public:
    virtual ~ILogSink() = default;

    virtual void write(const string& message) = 0;
};


// ============================================================
// Console Sink
// ============================================================

class ConsoleSink : public ILogSink {
private:
    mutex mtx;

public:
    void write(const string& message) override {
        lock_guard<mutex> lock(mtx);

        cout << message << '\n';
    }
};


// ============================================================
// File Sink
// ============================================================

class FileSink : public ILogSink {
private:
    ofstream file;
    mutex mtx;

public:
    explicit FileSink(const string& filename) {
        file.open(filename, ios::app);

        if (!file.is_open()) {
            throw runtime_error("Failed to open log file");
        }
    }

    ~FileSink() override {
        if (file.is_open()) {
            file.close();
        }
    }

    void write(const string& message) override {
        lock_guard<mutex> lock(mtx);

        if (!file.is_open()) {
            return;
        }

        file << message << '\n';
        file.flush();
    }
};


// ============================================================
// Logger
// ============================================================

class Logger {
private:
    LogLevel minLevel;

    vector<shared_ptr<ILogSink>> sinks;
    shared_ptr<ILogFormatter> formatter;

    mutex mtx;

    string currentTimestamp() const {
        auto now = chrono::system_clock::now();

        time_t time = chrono::system_clock::to_time_t(now);

        tm localTime{};

#ifdef _WIN32
        localtime_s(&localTime, &time);
#else
        localtime_r(&time, &localTime);
#endif

        ostringstream out;

        out << put_time(&localTime, "%Y-%m-%d %H:%M:%S");

        return out.str();
    }

public:
    Logger(
        LogLevel minLevel,
        shared_ptr<ILogFormatter> formatter
    )
        : minLevel(minLevel),
          formatter(std::move(formatter)) {

        if (!this->formatter) {
            throw invalid_argument("Formatter cannot be null");
        }
    }

    void addSink(shared_ptr<ILogSink> sink) {
        if (!sink) {
            throw invalid_argument("Sink cannot be null");
        }

        lock_guard<mutex> lock(mtx);

        sinks.push_back(std::move(sink));
    }

    void setLevel(LogLevel level) {
        lock_guard<mutex> lock(mtx);

        minLevel = level;
    }

    void log(LogLevel level, const string& message) {

        // Fast filtering.
        {
            lock_guard<mutex> lock(mtx);

            if (level < minLevel) {
                return;
            }
        }

        LogMessage logMessage{
            level,
            message,
            currentTimestamp(),
            this_thread::get_id()
        };

        string formattedMessage =
            formatter->format(logMessage);

        // Copy sinks so we don't hold Logger's mutex
        // while performing potentially slow I/O.
        vector<shared_ptr<ILogSink>> currentSinks;

        {
            lock_guard<mutex> lock(mtx);

            currentSinks = sinks;
        }

        for (const auto& sink : currentSinks) {
            try {
                sink->write(formattedMessage);
            }
            catch (const exception& e) {
                // Logging failure should not crash application.
                cerr << "Logging failure: "
                     << e.what() << '\n';
            }
        }
    }

    void debug(const string& message) {
        log(LogLevel::DEBUG, message);
    }

    void info(const string& message) {
        log(LogLevel::INFO, message);
    }

    void warn(const string& message) {
        log(LogLevel::WARN, message);
    }

    void error(const string& message) {
        log(LogLevel::ERROR, message);
    }

    void fatal(const string& message) {
        log(LogLevel::FATAL, message);
    }
};


// ============================================================
// Example
// ============================================================

int main() {

    auto formatter =
        make_shared<TextFormatter>();

    Logger logger(
        LogLevel::INFO,
        formatter
    );



    logger.addSink(
        make_shared<ConsoleSink>()
);
    logger.addSink(
        make_shared<FileSink>("application.log")
);


    logger.debug("Debug message");
    logger.info("Server started");
    logger.warn("Memory usage is high");
    logger.error("Database connection failed");
    logger.fatal("System shutting down");


    // Change minimum level.
    logger.setLevel(LogLevel::ERROR);

    logger.info("This will be ignored");

    logger.error("This will be logged");

    return 0;
}
