#include <iostream>
using namespace std;

class Formatter {
public:
    virtual string format(const string &message) = 0;
    virtual ~Formatter(){}
};

class PlainTextFormatter : public Formatter {
    string format(const string &message) {
        return message + "\n";
    }
};

class JSONFormatter : public Formatter {
    string format(const string &message) {
        string formattedMessage = "{ ";
        formattedMessage += "message: ";
        formattedMessage += message;
        formattedMessage += " }";
        formattedMessage += "\n";

        return formattedMessage;
    }
};

class Logger {
    Formatter* formatter;

public:
    Logger(Formatter* f) : formatter(f){}

    void log(string message) {
        if (formatter != nullptr) {
            cout << formatter->format(message);
        }
    }
};

int main() {
    string message = "Randi Rona";
    JSONFormatter jsonFormatter;
    Logger logger(&jsonFormatter);
    logger.log(message);

    PlainTextFormatter plainTextFormatter;
    Logger plainLogger(&plainTextFormatter);
    plainLogger.log(message);
}
