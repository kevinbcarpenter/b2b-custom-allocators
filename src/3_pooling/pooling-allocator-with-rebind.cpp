#include <memory>
#include <iostream>
#include <vector>
#include <list>

// Let's trace exactly when rebind gets called
template<typename T>
class TracingPoolAllocator {
public:
    using value_type = T;
    
    // This is the rebind mechanism - it creates allocators for different types
    template<typename U>
    struct rebind {
        using other = TracingPoolAllocator<U>;  // U is different from T!
    };
    
    TracingPoolAllocator() {
        std::cout << "TracingPoolAllocator<" << typeid(T).name() << "> created\n";
    }
    
    // Copy constructor for same type
    TracingPoolAllocator(const TracingPoolAllocator&) {
        std::cout << "TracingPoolAllocator<" << typeid(T).name() << "> copy constructed\n";
    }
    
    // Rebind constructor - this is where U comes from!
    template<typename U>
    TracingPoolAllocator(const TracingPoolAllocator<U>& other) {
        std::cout << "TracingPoolAllocator<" << typeid(T).name() 
                  << "> rebind constructed from TracingPoolAllocator<" 
                  << typeid(U).name() << ">\n";
    }
    
    T* allocate(std::size_t n) {
        std::cout << "Allocating " << n << " objects of type " << typeid(T).name() << "\n";
        return static_cast<T*>(std::malloc(n * sizeof(T)));
    }
    
    void deallocate(T* p, std::size_t n) noexcept {
        std::cout << "Deallocating " << n << " objects of type " << typeid(T).name() << "\n";
        std::free(p);
    }
    
    bool operator==(const TracingPoolAllocator&) const { return true; }
    bool operator!=(const TracingPoolAllocator&) const { return false; }
};

// Simple test class
struct MyObject {
    int value;
    MyObject(int v = 0) : value(v) {
        std::cout << "MyObject(" << value << ") created\n";
    }
    ~MyObject() {
        std::cout << "MyObject(" << value << ") destroyed\n";
    }
};

int main() {
    std::cout << "=== Understanding Rebind: When U != T ===\n\n";
    
    std::cout << "1. Creating TracingPoolAllocator<MyObject>:\n";
    TracingPoolAllocator<MyObject> my_alloc;
    
    std::cout << "\n2. Using with std::list<MyObject> (this will trigger rebind!):\n";
    std::cout << "   std::list needs to allocate list nodes, not just MyObject\n";
    
    try {
        // std::list<MyObject> has elements of type MyObject (T = MyObject)
        // But internally, it needs to allocate list nodes which have a different type!
        // Something like: struct ListNode { MyObject data; ListNode* next; ListNode* prev; }
        
        std::list<MyObject, TracingPoolAllocator<MyObject>> my_list(my_alloc);
        
        std::cout << "\n3. Adding elements to list:\n";
        my_list.emplace_back(42);
        my_list.emplace_back(84);
        
        std::cout << "\nList contents:\n";
        for (const auto& obj : my_list) {
            std::cout << "  Value: " << obj.value << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }
    
    std::cout << "\n=== What Happened Behind the Scenes ===\n";
    std::cout << "1. You created TracingPoolAllocator<MyObject> (T = MyObject)\n";
    std::cout << "2. std::list needs to allocate internal list nodes (U = ListNode)\n";
    std::cout << "3. std::list uses rebind to create TracingPoolAllocator<ListNode>\n";
    std::cout << "4. The rebind::other gives us TracingPoolAllocator<U> where U = ListNode\n";
    std::cout << "5. This is why you saw the rebind constructor called!\n";
    
    std::cout << "\n=== Manual Rebind Example ===\n";
    
    // Let's manually demonstrate rebind
    std::cout << "Original allocator: TracingPoolAllocator<MyObject>\n";
    TracingPoolAllocator<MyObject> original_alloc;
    
    // Now let's rebind it to allocate integers instead
    std::cout << "\nRebinding to TracingPoolAllocator<int>:\n";
    using ReboundAllocator = TracingPoolAllocator<MyObject>::rebind<int>::other;
    // ReboundAllocator is now TracingPoolAllocator<int>
    
    ReboundAllocator int_alloc(original_alloc);  // This calls the rebind constructor!
    
    // Now we can allocate ints with the rebound allocator
    std::cout << "\nUsing rebound allocator to allocate integers:\n";
    int* int_ptr = std::allocator_traits<ReboundAllocator>::allocate(int_alloc, 3);
    
    for (int i = 0; i < 3; ++i) {
        std::allocator_traits<ReboundAllocator>::construct(int_alloc, int_ptr + i, i * 10);
    }
    
    std::cout << "Allocated integers: ";
    for (int i = 0; i < 3; ++i) {
        std::cout << int_ptr[i] << " ";
    }
    std::cout << "\n";
    
    // Cleanup
    for (int i = 0; i < 3; ++i) {
        std::allocator_traits<ReboundAllocator>::destroy(int_alloc, int_ptr + i);
    }
    std::allocator_traits<ReboundAllocator>::deallocate(int_alloc, int_ptr, 3);
    
    return 0;
}