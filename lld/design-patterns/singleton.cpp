#include <iostream>
#include <bits/stdc++.h>
using namespace std;

// not thread safe
class LazySingleton {
private:
    static LazySingleton* instance;

    LazySingleton() = default;

public:
    static LazySingleton* getInstance() {
        if (instance != nullptr)
            return instance;

        return instance = new LazySingleton();
    }
};

LazySingleton* LazySingleton::instance = nullptr;

int main() {
    std::cout << (LazySingleton::getInstance() ==
                  LazySingleton::getInstance());
}


// thread safe
class Singleton {
private:
    Singleton() = default;

public:
    static Singleton& getInstance() {
        static Singleton instance;
        return instance;
    }
};

class DoubleCheckedSingleton {
private:
    // Holds the single shared instance (needs safe publication in C++)
    static DoubleCheckedSingleton* instance;
    // Lock used only during first-time creation
    static mutex lock;

    // Private constructor prevents external instantiation
    DoubleCheckedSingleton() {}

public:
    // Global access point to get the Singleton instance
    static DoubleCheckedSingleton* getInstance() {
        
		// Fast path: first check without locking
        if (instance == nullptr) {
            // Lock only when the instance might need to be created
            lock_guard<mutex> guard(lock);
            // Second check inside the lock (prevents double creation)
            if (instance == nullptr) {
                instance = new DoubleCheckedSingleton();
            }
        }

        // Return the shared instance
        return instance;
    }
};
