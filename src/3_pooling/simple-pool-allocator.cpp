#include <iostream>
#include <memory>
#include <cstddef>
#include <new>

template<typename T, size_t PoolSize = 8>
class SimplePoolAllocator {
private:
    // Pool storage - aligned for T
    alignas(T) char pool_[PoolSize * sizeof(T)];
    
    // Simple free list - stack of available blocks
    struct FreeNode {
        FreeNode* next;
    };
    
    FreeNode* free_head_ = nullptr;
    size_t allocated_count_ = 0;
    
    void initialize_pool() {
        // Initialize free list by threading through the pool
        char* current = pool_;
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            reinterpret_cast<FreeNode*>(current)->next = 
                reinterpret_cast<FreeNode*>(current + sizeof(T));
            current += sizeof(T);
        }
        // Last node points to nullptr
        reinterpret_cast<FreeNode*>(current)->next = nullptr;
        free_head_ = reinterpret_cast<FreeNode*>(pool_);
    }

public:
    // Standard allocator type definitions
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    // Constructor
    SimplePoolAllocator() {
        std::cout << "Initializing pool with " << PoolSize << " blocks of " 
                  << sizeof(T) << " bytes each" << std::endl;
        initialize_pool();
    }
    
    // Standard allocator interface - allocate n objects
    T* allocate(std::size_t n) {
        if (n != 1) {
            std::cout << "This simple pool only supports single object allocation" << std::endl;
            throw std::bad_alloc();
        }
        
        if (free_head_ == nullptr) {
            std::cout << "Pool exhausted!" << std::endl;
            throw std::bad_alloc();
        }
        
        // Pop from free list
        T* result = reinterpret_cast<T*>(free_head_);
        free_head_ = free_head_->next;
        ++allocated_count_;
        
        std::cout << "Allocated block #" << allocated_count_ 
                  << " at " << static_cast<void*>(result) << std::endl;
        
        return result;
    }
    
    // Standard allocator interface - deallocate n objects
    void deallocate(T* p, std::size_t n) {
        if (n != 1 || p == nullptr) return;
        
        // Simple bounds check
        if (p < reinterpret_cast<T*>(pool_) || 
            p >= reinterpret_cast<T*>(pool_ + sizeof(pool_))) {
            std::cout << "Warning: pointer not from this pool!" << std::endl;
            return;
        }
        
        // Push back to free list
        FreeNode* node = reinterpret_cast<FreeNode*>(p);
        node->next = free_head_;
        free_head_ = node;
        --allocated_count_;
        
        std::cout << "Deallocated block, " << allocated_count_ 
                  << " still allocated" << std::endl;
    }
    
    // Equality operators for allocator requirements
    bool operator==(const SimplePoolAllocator& other) const noexcept {
        return this == &other; // Same instance
    }
    
    bool operator!=(const SimplePoolAllocator& other) const noexcept {
        return !(*this == other);
    }
    
    // Statistics
    size_t allocated_count() const { return allocated_count_; }
    size_t available_count() const { return PoolSize - allocated_count_; }
    size_t pool_size() const { return PoolSize; }
};

// Test class for demonstration
struct TestObject {
    int id;
    double value;
    
    TestObject(int i = 0, double v = 0.0) : id(i), value(v) {
        std::cout << "TestObject(" << id << ", " << value << ") created" << std::endl;
    }
    
    ~TestObject() {
        std::cout << "TestObject(" << id << ", " << value << ") destroyed" << std::endl;
    }
};

int main() {
    std::cout << "=== Pool Allocator with allocator_traits Demo ===\n";
    std::cout << "This example shows standard-compliant allocation using allocator_traits\n\n";
    
    // Create a simple pool allocator for TestObject
    SimplePoolAllocator<TestObject> pool;
    
    std::cout << "Pool initialized with " << pool.pool_size() << " slots\n";
    std::cout << "Available slots: " << pool.available_count() << "\n\n";
    
    // Using allocator_traits - this is the recommended approach
    std::cout << "=== Using allocator_traits (standard approach) ===\n";
    TestObject* objects[5];
    
    // Shorthand for the allocator_traits type
    using AllocTraits = std::allocator_traits<SimplePoolAllocator<TestObject>>;
    
    for (int i = 0; i < 5; ++i) {
        // allocator_traits provides a standard interface
        objects[i] = AllocTraits::allocate(pool, 1);
        AllocTraits::construct(pool, objects[i], i + 1, (i + 1) * 1.5);
        
        std::cout << "Available after allocation " << (i+1) << ": " 
                  << pool.available_count() << " slots\n";
    }
    
    std::cout << "\n=== Using the objects ===\n";
    for (int i = 0; i < 5; ++i) {
        std::cout << "Object " << i << ": id=" << objects[i]->id 
                  << ", value=" << objects[i]->value << std::endl;
    }
    
    std::cout << "\n=== Cleaning up with allocator_traits ===\n";
    for (int i = 0; i < 5; ++i) {
        // allocator_traits handles destruction and deallocation
        AllocTraits::destroy(pool, objects[i]);
        AllocTraits::deallocate(pool, objects[i], 1);
        
        std::cout << "Available after deallocation " << (i+1) << ": " 
                  << pool.available_count() << " slots\n";
    }
    
    std::cout << "\n=== Testing pool exhaustion ===\n";
    try {
        // Try to allocate more than the pool size
        for (int i = 0; i < 10; ++i) {
            TestObject* obj = AllocTraits::allocate(pool, 1);
            AllocTraits::construct(pool, obj, 999, 999.9);
            std::cout << "Allocated extra object " << i << std::endl;
        }
    } catch (const std::bad_alloc& e) {
        std::cout << "Caught expected exception: pool exhausted\n";
    }
    
    std::cout << "\nFinal pool state - Available: " << pool.available_count() 
              << " / " << pool.pool_size() << std::endl;
    
    std::cout << "\nKey benefits of allocator_traits:\n";
    std::cout << "- Standard-compliant interface\n";
    std::cout << "- Provides default construct/destroy implementations\n";
    std::cout << "- Less template code to write\n";
    std::cout << "- Compatible with standard containers (if rebind added)\n";
    std::cout << "- Forward compatible with future C++ standards\n";
    
    return 0;
}