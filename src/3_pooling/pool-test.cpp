#include <memory>
#include <iostream>
#include <vector>
#include <cstddef>

template<typename T, size_t PoolSize = 64>
class PoolAllocator {
private:
    alignas(T) char pool_[PoolSize * sizeof(T)];
    
    struct FreeNode {
        FreeNode* next;
    };
    
    FreeNode* free_head_ = nullptr;

public:
    using value_type = T;
    
    // Required for C++11/14 compatibility
    template<typename U>
    struct rebind {
        using other = PoolAllocator<U, PoolSize>;
    };
    
    PoolAllocator() {
        // Initialize free list
        char* current = pool_;
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            reinterpret_cast<FreeNode*>(current)->next = 
                reinterpret_cast<FreeNode*>(current + sizeof(T));
            current += sizeof(T);
        }
        reinterpret_cast<FreeNode*>(current)->next = nullptr;
        free_head_ = reinterpret_cast<FreeNode*>(pool_);
    }
    
    // Copy constructor (each allocator has its own pool)
    PoolAllocator(const PoolAllocator&) : PoolAllocator() {}
    
    // Rebind constructor
    template<typename U>
    PoolAllocator(const PoolAllocator<U, PoolSize>&) : PoolAllocator() {}
    
    T* allocate(std::size_t n) {
        if (n != 1 || !free_head_) {
            throw std::bad_alloc();
        }
        
        T* result = reinterpret_cast<T*>(free_head_);
        free_head_ = free_head_->next;
        return result;
    }
    
    void deallocate(T* p, std::size_t n) noexcept {
        if (n != 1 || !p) return;
        
        FreeNode* node = reinterpret_cast<FreeNode*>(p);
        node->next = free_head_;
        free_head_ = node;
    }
    
    bool operator==(const PoolAllocator& other) const noexcept {
        return this == &other;
    }
    
    bool operator!=(const PoolAllocator& other) const noexcept {
        return !(*this == other);
    }
};

struct TestObject {
    int id;
    double value;
    
    TestObject(int i = 0, double v = 0.0) : id(i), value(v) {
        std::cout << "TestObject(" << id << ", " << value << ") created\n";
    }
    
    ~TestObject() {
        std::cout << "TestObject(" << id << ", " << value << ") destroyed\n";
    }
};

int main() {
    std::cout << "=== Simple Pool Allocator with allocator_traits ===\n\n";
    
    PoolAllocator<TestObject> pool_alloc;
    
    // Allocate and construct objects using allocator_traits
    std::vector<TestObject*> objects;
    
    for (int i = 1; i <= 3; ++i) {
        // Modern way: use allocator_traits
        TestObject* obj = std::allocator_traits<PoolAllocator<TestObject>>::allocate(pool_alloc, 1);
        std::allocator_traits<PoolAllocator<TestObject>>::construct(pool_alloc, obj, i, i * 1.5);
        objects.push_back(obj);
    }
    
    std::cout << "\n=== Using objects ===\n";
    for (auto* obj : objects) {
        std::cout << "Object: id=" << obj->id << ", value=" << obj->value << "\n";
    }
    
    std::cout << "\n=== Cleaning up ===\n";
    // Clean up using allocator_traits
    for (auto* obj : objects) {
        std::allocator_traits<PoolAllocator<TestObject>>::destroy(pool_alloc, obj);
        std::allocator_traits<PoolAllocator<TestObject>>::deallocate(pool_alloc, obj, 1);
    }
    
    return 0;
}