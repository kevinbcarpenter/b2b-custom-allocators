#include <iostream>
#include <memory>
#include <cstddef>
#include <new>

// Shared state for all stack allocator instances
// This allows different template instantiations to share the same underlying stack
struct StackState {
    char* memory_;
    size_t total_size_;
    size_t current_offset_;
    size_t ref_count_;
    
    StackState(size_t size) 
        : total_size_(size), current_offset_(0), ref_count_(1) {
        memory_ = new char[size];
        std::cout << "Stack allocator created with " << size << " bytes\n";
    }
    
    ~StackState() {
        delete[] memory_;
        std::cout << "Stack allocator destroyed\n";
    }
};

// Stack Allocator with standard allocator interface
// This version is compatible with allocator_traits while maintaining
// the LIFO (Last In, First Out) allocation behavior of stack allocators
template<typename T>
class StackAllocator {
private:
    StackState* state_;
    
public:
    // Standard allocator type definitions
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    
    // Constructor - creates a new stack with specified size
    explicit StackAllocator(size_t size = 1024) {
        state_ = new StackState(size);
    }
    
    // Copy constructor - shares the same stack
    StackAllocator(const StackAllocator& other) : state_(other.state_) {
        ++state_->ref_count_;
    }
    
    // Rebind constructor for different types (shares same stack)
    template<typename U>
    StackAllocator(const StackAllocator<U>& other) : state_(other.state_) {
        ++state_->ref_count_;
    }
    
    // Assignment operator
    StackAllocator& operator=(const StackAllocator& other) {
        if (this != &other) {
            // Decrease ref count on current state
            if (--state_->ref_count_ == 0) {
                delete state_;
            }
            // Share the other's state
            state_ = other.state_;
            ++state_->ref_count_;
        }
        return *this;
    }
    
    // Destructor
    ~StackAllocator() {
        if (--state_->ref_count_ == 0) {
            delete state_;
        }
    }
    
    // Standard allocator interface - allocate n objects
    T* allocate(std::size_t n) {
        size_t bytes = n * sizeof(T);
        size_t alignment = alignof(T);
        
        // Calculate properly aligned offset
        size_t aligned_offset = align_up(state_->current_offset_, alignment);
        
        // Check if we have enough space
        if (aligned_offset + bytes > state_->total_size_) {
            std::cout << "Stack allocator out of memory! Requested: " << bytes 
                      << " bytes, available: " << (state_->total_size_ - aligned_offset) << " bytes\n";
            throw std::bad_alloc();
        }
        
        // Return pointer to the allocated memory
        T* ptr = reinterpret_cast<T*>(state_->memory_ + aligned_offset);
        state_->current_offset_ = aligned_offset + bytes;
        
        std::cout << "Allocated " << n << " objects of " << sizeof(T) 
                  << " bytes each at offset " << aligned_offset 
                  << " (stack top now at " << state_->current_offset_ << ")\n";
        
        return ptr;
    }
    
    // Standard allocator interface - deallocate n objects
    // Note: Stack allocators typically don't support individual deallocation
    // This is a no-op to maintain standard compliance
    void deallocate(T* p, std::size_t n) noexcept {
        // Stack allocators don't support individual deallocation
        // In a real implementation, you might check if this is the last allocation
        // and adjust the stack pointer accordingly, but for simplicity we do nothing
        (void)p; (void)n;  // Suppress unused parameter warnings
        std::cout << "Deallocate called (no-op for stack allocator)\n";
    }
    
    // Equality operators (required for standard allocators)
    bool operator==(const StackAllocator& other) const noexcept {
        return state_ == other.state_;  // Same underlying stack
    }
    
    bool operator!=(const StackAllocator& other) const noexcept {
        return !(*this == other);
    }
    
    // Stack-specific methods
    
    // Get current stack position (for creating markers)
    size_t get_marker() const {
        return state_->current_offset_;
    }
    
    // Reset stack to a previous marker
    void free_to_marker(size_t marker) {
        if (marker > state_->current_offset_) {
            std::cout << "Warning: Invalid marker " << marker 
                      << " (current offset is " << state_->current_offset_ << ")\n";
            return;
        }
        
        size_t freed_bytes = state_->current_offset_ - marker;
        state_->current_offset_ = marker;
        
        std::cout << "Freed " << freed_bytes << " bytes, stack top now at " 
                  << state_->current_offset_ << "\n";
    }
    
    // Clear entire stack
    void clear() {
        size_t freed_bytes = state_->current_offset_;
        state_->current_offset_ = 0;
        std::cout << "Cleared entire stack, freed " << freed_bytes << " bytes\n";
    }
    
    // Statistics methods
    size_t get_remaining_size() const { return state_->total_size_ - state_->current_offset_; }
    size_t get_total_size() const { return state_->total_size_; }
    size_t get_used_size() const { return state_->current_offset_; }
    
    // Allow access to state for rebind constructor
    template<typename U>
    friend class StackAllocator;
    
private:
    // Helper function to align a value up to the specified alignment
    static size_t align_up(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

// Test object to demonstrate construction/destruction with allocator_traits
struct TestObject {
    int id;
    double value;
    
    TestObject(int i = 0, double v = 0.0) : id(i), value(v) {
        std::cout << "  TestObject(" << id << ", " << value << ") constructed\n";
    }
    
    ~TestObject() {
        std::cout << "  TestObject(" << id << ", " << value << ") destroyed\n";
    }
};

int main() {
    std::cout << "=== Stack Allocator with allocator_traits Demo ===\n";
    std::cout << "This example shows a standard-compliant stack allocator\n\n";
    
    // Create a stack allocator for TestObject with 1KB
    StackAllocator<TestObject> allocator(1024);
    
    // Use allocator_traits for standard operations
    using AllocTraits = std::allocator_traits<StackAllocator<TestObject>>;
    
    std::cout << "\n=== Basic allocation with allocator_traits ===\n";
    
    // Allocate and construct objects using allocator_traits
    TestObject* objects = AllocTraits::allocate(allocator, 3);
    
    for (int i = 0; i < 3; ++i) {
        AllocTraits::construct(allocator, &objects[i], i + 1, (i + 1) * 2.5);
    }
    
    std::cout << "Objects created and constructed:\n";
    for (int i = 0; i < 3; ++i) {
        std::cout << "  Object " << i << ": id=" << objects[i].id 
                  << ", value=" << objects[i].value << "\n";
    }
    
    std::cout << "Current usage: " << allocator.get_used_size() 
              << " / " << allocator.get_total_size() << " bytes\n";
    
    std::cout << "\n=== Marker-based management ===\n";
    
    // Save a checkpoint
    size_t checkpoint = allocator.get_marker();
    std::cout << "Saved checkpoint at offset " << checkpoint << "\n";
    
    // Allocate more objects
    TestObject* temp_objects = AllocTraits::allocate(allocator, 2);
    for (int i = 0; i < 2; ++i) {
        AllocTraits::construct(allocator, &temp_objects[i], 100 + i, (100 + i) * 0.1);
    }
    
    std::cout << "Peak usage: " << allocator.get_used_size() 
              << " / " << allocator.get_total_size() << " bytes\n";
    
    // Free back to checkpoint (this invalidates temp_objects!)
    std::cout << "\nFreeing back to checkpoint...\n";
    // Note: In a real scenario, you'd need to manually destroy the temp objects first
    for (int i = 1; i >= 0; --i) {
        AllocTraits::destroy(allocator, &temp_objects[i]);
    }
    allocator.free_to_marker(checkpoint);
    
    // Original objects are still valid
    std::cout << "Original objects still valid:\n";
    for (int i = 0; i < 3; ++i) {
        std::cout << "  Object " << i << ": id=" << objects[i].id 
                  << ", value=" << objects[i].value << "\n";
    }
    
    std::cout << "\n=== Working with different types ===\n";
    
    // Create allocator for a different type (shares same underlying stack)
    StackAllocator<int> int_allocator(allocator);
    using IntAllocTraits = std::allocator_traits<StackAllocator<int>>;
    
    // Allocate integers
    int* integers = IntAllocTraits::allocate(int_allocator, 5);
    for (int i = 0; i < 5; ++i) {
        IntAllocTraits::construct(int_allocator, &integers[i], i * i);
    }
    
    std::cout << "Allocated integers: ";
    for (int i = 0; i < 5; ++i) {
        std::cout << integers[i] << " ";
    }
    std::cout << "\n";
    
    std::cout << "Final usage: " << allocator.get_used_size() 
              << " / " << allocator.get_total_size() << " bytes\n";
    
    std::cout << "\n=== Cleanup ===\n";
    
    // Destroy all objects (in reverse order of construction)
    for (int i = 4; i >= 0; --i) {
        IntAllocTraits::destroy(int_allocator, &integers[i]);
    }
    for (int i = 2; i >= 0; --i) {
        AllocTraits::destroy(allocator, &objects[i]);
    }
    
    // Clear the stack
    allocator.clear();
    
    std::cout << "\nKey benefits of allocator_traits version:\n";
    std::cout << "- Standard-compliant interface\n";
    std::cout << "- Can work with standard containers (with limitations)\n";
    std::cout << "- allocator_traits provides default construct/destroy\n";
    std::cout << "- Type safety through templates\n";
    std::cout << "- Rebind support for different types on same stack\n";
    std::cout << "\nLimitations:\n";
    std::cout << "- Individual deallocate() is a no-op\n";
    std::cout << "- Best used with manual marker management\n";
    std::cout << "- Requires careful object lifetime management\n";
    
    return 0;
}