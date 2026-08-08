#include <bits/stdc++.h>
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

class NotificationCreator {
public:
    // Factory Method - subclasses decide what to create
    virtual unique_ptr<Notification> createNotification() = 0;

    // Shared logic that uses the factory method
    void send(const string& message) {
        auto notification = createNotification();
        notification->send(message);
    }

    virtual ~NotificationCreator() = default;
};

class EmailNotificationCreator : public NotificationCreator {
public:
    unique_ptr<Notification> createNotification() override {
        return make_unique<EmailNotification>();
    }
};

class SMSNotificationCreator : public NotificationCreator {
public:
    unique_ptr<Notification> createNotification() override {
        return make_unique<SMSNotification>();
    }
};

class PushNotificationCreator : public NotificationCreator {
public:
    unique_ptr<Notification> createNotification() override {
        return make_unique<PushNotification>();
    }
};

class SlackNotificationCreator : public NotificationCreator {
public:
    unique_ptr<Notification> createNotification() override {
        return make_unique<SlackNotification>();
    }
};

int main() {
    // Send Email
    unique_ptr<NotificationCreator> creator = make_unique<EmailNotificationCreator>();
    creator->send("Welcome to our platform!");

    // Send SMS
    creator = make_unique<SMSNotificationCreator>();
    creator->send("Your OTP is 123456");

    // Send Push Notification
    creator = make_unique<PushNotificationCreator>();
    creator->send("You have a new follower!");

    // Send Slack Message
    creator = make_unique<SlackNotificationCreator>();
    creator->send("Standup in 10 minutes!");

    EmailNotificationCreator enc;
    enc.send("Hello");

    return 0;
}
