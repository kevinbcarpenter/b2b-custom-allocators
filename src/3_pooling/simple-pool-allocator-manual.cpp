#include <iostream>
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
    // Constructor
    SimplePoolAllocator() {
        std::cout << "Initializing pool with " << PoolSize << " blocks of " 
                  << sizeof(T) << " bytes each" << std::endl;
        initialize_pool();
    }
    
    // Allocate one object from the pool
    T* allocate() {
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
    
    // Deallocate one object back to the pool
    void deallocate(T* p) {
        if (p == nullptr) return;
        
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
    std::cout << "=== Manual Pool Allocator Demo ===\n";
    std::cout << "This example shows direct allocation with manual object construction\n\n";
    
    // Create a simple pool allocator for TestObject
    SimplePoolAllocator<TestObject> pool;
    
    std::cout << "Pool initialized with " << pool.pool_size() << " slots\n";
    std::cout << "Available slots: " << pool.available_count() << "\n\n";
    
    // Allocate and construct some objects manually
    TestObject* objects[5];
    
    std::cout << "=== Manual allocation and construction ===\n";
    for (int i = 0; i < 5; ++i) {
        // Step 1: Allocate raw memory from pool
        objects[i] = pool.allocate();
        
        // Step 2: Construct object using placement new
        new(objects[i]) TestObject(i + 1, (i + 1) * 1.5);
        
        std::cout << "Available after allocation " << (i+1) << ": " 
                  << pool.available_count() << " slots\n";
    }
    
    std::cout << "\n=== Using the objects ===\n";
    for (int i = 0; i < 5; ++i) {
        std::cout << "Object " << i << ": id=" << objects[i]->id 
                  << ", value=" << objects[i]->value << std::endl;
    }
    
    std::cout << "\n=== Manual destruction and deallocation ===\n";
    for (int i = 0; i < 5; ++i) {
        // Step 1: Manually call destructor
        objects[i]->~TestObject();
        
        // Step 2: Return memory to pool
        pool.deallocate(objects[i]);
        
        std::cout << "Available after deallocation " << (i+1) << ": " 
                  << pool.available_count() << " slots\n";
    }
    
    std::cout << "\n=== Testing pool exhaustion ===\n";
    try {
        // Try to allocate more than the pool size
        for (int i = 0; i < 10; ++i) {
            TestObject* obj = pool.allocate();
            new(obj) TestObject(999, 999.9);
            std::cout << "Allocated extra object " << i << std::endl;
        }
    } catch (const std::bad_alloc& e) {
        std::cout << "Caught expected exception: pool exhausted\n";
    }
    
    std::cout << "\nFinal pool state - Available: " << pool.available_count() 
              << " / " << pool.pool_size() << std::endl;
    
    std::cout << "\nKey points:\n";
    std::cout << "- Direct allocation with allocate() / deallocate()\n";
    std::cout << "- Manual object construction with placement new\n";
    std::cout << "- Manual destructor calls\n";
    std::cout << "- Simple, easy to understand for beginners\n";
    
    return 0;
}