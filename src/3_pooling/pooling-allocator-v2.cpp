#include <memory>
#include <vector>
#include <iostream>
#include <cstddef>
#include <cassert>

template<typename T, size_t PoolSize = 1024>
class PoolAllocator {
private:
    // Pool storage - aligned for T
    alignas(T) char pool_[PoolSize * sizeof(T)];
    
    // Free list - simple stack of available blocks
    struct FreeNode {
        FreeNode* next;
    };
    
    FreeNode* free_head_ = nullptr;
    size_t allocated_count_ = 0;
    
    void initialize_pool() {
        // Initialize free list - thread through the pool
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
    // Required type aliases for allocator
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    // C++17 and later: rebind is automatically provided by allocator_traits
    // But for older compilers, we need to provide it explicitly
    template<typename U>
    struct rebind {
        using other = PoolAllocator<U, PoolSize>;
    };
    
    // Constructor
    PoolAllocator() {
        initialize_pool();
    }
    
    // Copy constructor (required for container compatibility)
    PoolAllocator(const PoolAllocator& other) noexcept {
        initialize_pool(); // Each allocator has its own pool
    }
    
    // Rebind constructor (for different types)
    template<typename U>
    PoolAllocator(const PoolAllocator<U, PoolSize>&) {
        initialize_pool();
    }
    
    // Allocate memory
    T* allocate(std::size_t n) {
        if (n != 1) {
            throw std::bad_alloc(); // This simple pool only handles single objects
        }
        
        if (free_head_ == nullptr) {
            throw std::bad_alloc(); // Pool exhausted
        }
        
        // Pop from free list
        T* result = reinterpret_cast<T*>(free_head_);
        free_head_ = free_head_->next;
        ++allocated_count_;
        
        std::cout << "Pool allocated block #" << allocated_count_ 
                  << " at " << static_cast<void*>(result) << std::endl;
        
        return result;
    }
    
    // Deallocate memory
    void deallocate(T* p, std::size_t n) noexcept {
        if (n != 1 || p == nullptr) return;
        
        // Verify pointer is within our pool
        if (p < reinterpret_cast<T*>(pool_) || 
            p >= reinterpret_cast<T*>(pool_ + sizeof(pool_))) {
            return; // Not our memory, ignore
        }
        
        // Push back to free list
        FreeNode* node = reinterpret_cast<FreeNode*>(p);
        node->next = free_head_;
        free_head_ = node;
        --allocated_count_;
        
        std::cout << "Pool deallocated block, " << allocated_count_ 
                  << " still allocated" << std::endl;
    }
    
    // Optional: construct (allocator_traits will provide default if missing)
    template<typename... Args>
    void construct(T* p, Args&&... args) {
        std::cout << "Pool constructing object" << std::endl;
        ::new(p) T(std::forward<Args>(args)...);
    }
    
    // Optional: destroy (allocator_traits will provide default if missing)
    void destroy(T* p) noexcept {
        std::cout << "Pool destroying object" << std::endl;
        p->~T();
    }
    
    // Equality comparison (required)
    bool operator==(const PoolAllocator& other) const noexcept {
        return this == &other; // Same pool instance
    }
    
    bool operator!=(const PoolAllocator& other) const noexcept {
        return !(*this == other);
    }
    
    // Statistics
    size_t allocated_count() const { return allocated_count_; }
    size_t available_count() const { return PoolSize - allocated_count_; }
    constexpr size_t pool_size() const { return PoolSize; }
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
    std::cout << "=== Pool Allocator Demo ===\n\n";
    
    // Create pool allocator for TestObject with pool size of 8
    PoolAllocator<TestObject, 8> pool_alloc;
    
    std::cout << "Pool size: " << pool_alloc.pool_size() << std::endl;
    std::cout << "Available: " << pool_alloc.available_count() << std::endl << std::endl;
    
    // Allocate and construct some objects using modern allocator_traits
    std::vector<TestObject*> objects;
    
    for (int i = 1; i <= 5; ++i) {
        // Modern way: use allocator_traits
        TestObject* obj = std::allocator_traits<PoolAllocator<TestObject, 8>>::allocate(pool_alloc, 1);
        std::allocator_traits<PoolAllocator<TestObject, 8>>::construct(pool_alloc, obj, i, i * 1.5);
        objects.push_back(obj);
        
        std::cout << "Available after allocation: " << pool_alloc.available_count() << std::endl;
    }
    
    std::cout << "\n=== Using objects ===\n";
    for (auto* obj : objects) {
        std::cout << "Object: id=" << obj->id << ", value=" << obj->value << std::endl;
    }
    
    std::cout << "\n=== Cleaning up ===\n";
    // Clean up using allocator_traits
    for (auto* obj : objects) {
        std::allocator_traits<PoolAllocator<TestObject, 8>>::destroy(pool_alloc, obj);
        std::allocator_traits<PoolAllocator<TestObject, 8>>::deallocate(pool_alloc, obj, 1);
    }
    
    std::cout << "\nFinal available: " << pool_alloc.available_count() << std::endl;
    
    std::cout << "\n=== Testing with std::vector ===\n";
    // Using the pool allocator with a standard container
    std::vector<TestObject, PoolAllocator<TestObject, 8>> pool_vector(pool_alloc);
    
    try {
        pool_vector.emplace_back(100, 99.9);
        pool_vector.emplace_back(200, 199.9);
        
        std::cout << "Vector contents:\n";
        for (const auto& obj : pool_vector) {
            std::cout << "  id=" << obj.id << ", value=" << obj.value << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}