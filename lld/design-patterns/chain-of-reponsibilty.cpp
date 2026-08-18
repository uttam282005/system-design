class Handler {
protected:
    Handler* next = nullptr;

public:
    void setNext(Handler* handler) {
        next = handler;
    }

    virtual void handle(int request) = 0;

    virtual ~Handler() = default;
};

class Level1Support : public Handler {
public:
    void handle(int request) override {
        if (request <= 10) {
            cout << "Level 1 handled request\n";
        }
        else if (next) {
            next->handle(request);
        }
    }
};

class Level2Support : public Handler {
public:
    void handle(int request) override {
        if (request <= 50) {
            cout << "Level 2 handled request\n";
        }
        else if (next) {
            next->handle(request);
        }
    }
};

class Manager : public Handler {
public:
    void handle(int request) override {
        cout << "Manager handled request\n";
    }
};

Level1Support level1;
Level2Support level2;
Manager manager;

level1.setNext(&level2);
level2.setNext(&manager);

level1.handle(40);
