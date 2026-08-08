#include <bits/stdc++.h>
#include <memory>
using namespace std;

class Notification {
public:
    virtual void send(const string& message) = 0;
    virtual ~Notification() {}
};

class EmailNotification : public Notification {
public:
    void send(const string& message) override {
        cout << "Sending email: " << message << endl;
    }
};

class SMSNotification : public Notification {
public:
    void send(const string& message) override {
        cout << "Sending SMS: " << message << endl;
    }
};

class PushNotification : public Notification {
public:
    void send(const string& message) override {
        cout << "Sending push notification: " << message << endl;
    }
};

class SlackNotification : public Notification {
public:
    void send(const string& message) override {
        cout << "Sending Slack message: " << message << endl;
    }
};

class NotificationSender {
private:
    Notification* notification;

public:
    NotificationSender(Notification* n) : notification(n) {} 
    void send(const string& message) {
        if (notification)
            notification->send(message);
    }
};

int main() {
    EmailNotification email;
    NotificationSender ens(&email);

    ens.send("Hello");
}
