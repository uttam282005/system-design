#include <bits/stdc++.h>
using namespace std;

const int MAX_TTL = 1000000;
const int NO_TTL = -1;

class TTL {
private:
    int value;
public:
    TTL(int sec) : value(sec){};
};

template <typename T>
class Key {
    T value;
    TTL ttl = NO_TTL;

public:
    Key(T v) : value(v){};

    bool setTTL(int sec) {
        if (sec > MAX_TTL) return false; 
        ttl = sec;

        return true;
    }

    T getValue() {
        return value;
    }
};

template <typename T>
class Value {
    T value;

public:
    Value(T v) : value(v){};

    T getValue() {
        return value;
    }
};

template <typename K, typename V>
class MemoryStore {
public:
    virtual ~MemoryStore() = default;

    virtual bool put(const K& key, const V& value) = 0;

    virtual std::optional<V> get(const K& key) = 0;

    virtual bool remove(const K& key) = 0;

    virtual bool contains(const K& key) = 0;

    virtual size_t size() const = 0;
};

template <typename K, typename V>
class LRUCache {
private:
    MemoryStore<K, V>& ms;
public:
    LRUCache() = default;

    bool put(K k, V v) {
        Key<K> key(k);
        Value<V> value(v);

        ms.put(key, value);
    }
};
