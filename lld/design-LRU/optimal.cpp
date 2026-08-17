#include <bits/stdc++.h>
using namespace std;

template <typename K, typename V>
struct Node {
    K key;
    V value;
    Node* prev;
    Node* next;

    Node(K k, V v) : key(k), value(v), prev(nullptr), next(nullptr) {}
};

template <typename K, typename V>
class DoublyLinkedList {
private:
    Node<K, V>* head;
    Node<K, V>* tail;

public:
    DoublyLinkedList() {
        // Create dummy nodes with default-constructed key and value
        head = new Node<K, V>(K{}, V{});
        tail = new Node<K, V>(K{}, V{});

        // Link them together
        head->next = tail;
        tail->prev = head;
    }

    ~DoublyLinkedList() {
        // Only delete the dummy nodes
        // Real data nodes are managed by LRUCache
        delete head;
        delete tail;
    }

    void addFirst(Node<K, V>* node) {
        // Insert node between head and head->next
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }

    void remove(Node<K, V>* node) {
        // Bypass this node by linking its neighbors
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToFront(Node<K, V>* node) {
        remove(node);
        addFirst(node);
    }

    Node<K, V>* removeLast() {
        // Check if list is empty (only dummies present)
        if (tail->prev == head) {
            return nullptr;
        }

        Node<K, V>* last = tail->prev;
        remove(last);
        return last;
    }
};

template <typename K, typename V>
class LRUCache {
private:
    int capacity;
    std::unordered_map<K, Node<K, V>*> map;
    DoublyLinkedList<K, V> list;
    mutable std::mutex mtx;

public:
    LRUCache(int cap) : capacity(cap) {}

    ~LRUCache() {
        // Clean up all nodes stored in the map
        for (auto& pair : map) {
            delete pair.second;
        }
    }

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mtx);

        auto it = map.find(key);
        if (it == map.end()) {
            return std::nullopt;
        }

        Node<K, V>* node = it->second;
        list.moveToFront(node);
        return node->value;
    }

    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mtx);

        auto it = map.find(key);
        if (it != map.end()) {
            // Update existing entry
            Node<K, V>* node = it->second;
            node->value = value;
            list.moveToFront(node);
        } else {
            // Add new entry
            if (static_cast<int>(map.size()) == capacity) {
                // Evict LRU item
                Node<K, V>* lru = list.removeLast();
                if (lru != nullptr) {
                    map.erase(lru->key);
                    delete lru;
                }
            }

            Node<K, V>* newNode = new Node<K, V>(key, value);
            list.addFirst(newNode);
            map[key] = newNode;
        }
    }
};

int main() {
    cout << "=== LRU Cache Demo ===" << endl << endl;

    LRUCache<string, int> cache(3);

    cout << "1. Adding items to cache (capacity = 3)" << endl;
    cache.put("a", 1);
    cout << "   put('a', 1)" << endl;
    cache.put("b", 2);
    cout << "   put('b', 2)" << endl;
    cache.put("c", 3);
    cout << "   put('c', 3)" << endl;
    cout << "   Cache state: {a=1, b=2, c=3}" << endl;

    cout << endl << "2. Accessing 'a' makes it most recently used" << endl;
    auto valueA = cache.get("a");
    cout << "   get('a') = " << (valueA.has_value() ? to_string(valueA.value()) : "nullopt") << endl;
    cout << "   Order now: b (LRU) -> c -> a (MRU)" << endl;

    cout << endl << "3. Adding 'd' should evict 'b' (the LRU item)" << endl;
    cache.put("d", 4);
    cout << "   put('d', 4)" << endl;
    auto valueB = cache.get("b");
    cout << "   get('b') = " << (valueB.has_value() ? to_string(valueB.value()) : "nullopt")
              << " (nullopt means evicted)" << endl;

    cout << endl << "4. Verifying other items still accessible" << endl;
    auto valC = cache.get("c");
    auto valA = cache.get("a");
    auto valD = cache.get("d");
    cout << "   get('c') = " << (valC.has_value() ? to_string(valC.value()) : "nullopt") << endl;
    cout << "   get('a') = " << (valA.has_value() ? to_string(valA.value()) : "nullopt") << endl;
    cout << "   get('d') = " << (valD.has_value() ? to_string(valD.value()) : "nullopt") << endl;

    cout << endl << "5. Updating existing key" << endl;
    cache.put("c", 30);
    cout << "   put('c', 30) - updates value and marks as MRU" << endl;
    auto updatedC = cache.get("c");
    cout << "   get('c') = " << (updatedC.has_value() ? to_string(updatedC.value()) : "nullopt") << endl;

    cout << endl << "6. Adding 'e' should evict 'a' (now the LRU)" << endl;
    cache.put("e", 5);
    cout << "   put('e', 5)" << endl;
    auto evictedA = cache.get("a");
    auto stillD = cache.get("d");
    cout << "   get('a') = " << (evictedA.has_value() ? to_string(evictedA.value()) : "nullopt")
              << " (nullopt means evicted)" << endl;
    cout << "   get('d') = " << (stillD.has_value() ? to_string(stillD.value()) : "nullopt") << endl;

    cout << endl << "=== Demo Complete ===" << endl;

    return 0;
}
