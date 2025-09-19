#include <iostream>
#include <cstddef>
#include <cassert>
#include <new>

// Stack Allocator - allocates memory in LIFO (Last In, First Out) order
// Key characteristics:
// - Very fast allocation (just increment a pointer)
// - No individual deallocation - only bulk deallocation via markers
// - Excellent cache locality
// - Perfect for temporary allocations with known lifetimes
class StackAllocator {
private:
    char* memory_;          // Pointer to the allocated memory block
    std::size_t total_size_;     // Total size of the memory block
    std::size_t current_offset_; // Current top of stack (next allocation position)
    
public:
    // Constructor - allocates a large block of memory upfront
    explicit StackAllocator(std::size_t size) 
        : total_size_(size), current_offset_(0) {
        memory_ = new char[size];
        std::cout << "Stack allocator created with " << size << " bytes\n";
    }
    
    // Destructor - frees the entire memory block
    ~StackAllocator() {
        delete[] memory_;
        std::cout << "Stack allocator destroyed\n";
    }
    
    // Prevent copying - each allocator should own its memory
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    
    // Allocate raw memory from the stack
    void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        // Calculate properly aligned offset
        std::size_t aligned_offset = align_up(current_offset_, alignment);
        
        // Check if we have enough space
        if (aligned_offset + bytes > total_size_) {
            std::cout << "Stack allocator out of memory! Requested: " << bytes 
                      << " bytes, available: " << (total_size_ - aligned_offset) << " bytes\n";
            throw std::bad_alloc();
        }
        
        // Return pointer to the allocated memory
        void* ptr = memory_ + aligned_offset;
        current_offset_ = aligned_offset + bytes;
        
        std::cout << "Allocated " << bytes << " bytes at offset " << aligned_offset 
                  << " (stack top now at " << current_offset_ << ")\n";
        
        return ptr;
    }
    
    // Type-safe allocation template
    template<typename T>
    T* allocate(std::size_t count = 1) {
        std::size_t bytes = sizeof(T) * count;
        void* ptr = allocate(bytes, alignof(T));
        return static_cast<T*>(ptr);
    }
    
    // Get current stack position (for creating markers)
    std::size_t get_marker() const {
        return current_offset_;
    }
    
    // Reset stack to a previous marker (frees everything allocated after that point)
    void free_to_marker(std::size_t marker) {
        if (marker > current_offset_) {
            std::cout << "Warning: Invalid marker " << marker 
                      << " (current offset is " << current_offset_ << ")\n";
            return;
        }
        
        std::size_t freed_bytes = current_offset_ - marker;
        current_offset_ = marker;
        
        std::cout << "Freed " << freed_bytes << " bytes, stack top now at " 
                  << current_offset_ << "\n";
    }
    
    // Clear entire stack (reset to beginning)
    void clear() {
        std::size_t freed_bytes = current_offset_;
        current_offset_ = 0;
        std::cout << "Cleared entire stack, freed " << freed_bytes << " bytes\n";
    }
    
    // Statistics methods
    std::size_t get_remaining_size() const { return total_size_ - current_offset_; }
    std::size_t get_total_size() const { return total_size_; }
    std::size_t get_used_size() const { return current_offset_; }
    
private:
    // Helper function to align a value up to the specified alignment
    static std::size_t align_up(std::size_t value, std::size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

// Test object to demonstrate construction/destruction
struct TestObject {
    int id;
    double value;
    
    TestObject(int i, double v) : id(i), value(v) {
        std::cout << "  TestObject(" << id << ", " << value << ") constructed\n";
    }
    
    ~TestObject() {
        std::cout << "  TestObject(" << id << ", " << value << ") destroyed\n";
    }
};

int main() {
    std::cout << "=== Stack Allocator Demo ===\n\n";
    
    // Create a 512-byte stack allocator
    StackAllocator allocator(512);
    
    std::cout << "\n=== Basic allocation demo ===\n";
    
    // Allocate some integers
    int* integers = allocator.allocate<int>(7);
    for (int i = 0; i < 7; ++i) {
        integers[i] = i * i;
    }
    
    std::cout << "Allocated integers: ";
    for (int i = 0; i < 7; ++i) {
        std::cout << integers[i] << " ";
    }
    std::cout << "\n";
    
    // Show current usage
    std::cout << "Current usage: " << allocator.get_used_size() 
              << " / " << allocator.get_total_size() << " bytes\n";
    
    std::cout << "\n=== Marker-based deallocation demo ===\n";
    
    // Save a marker before allocating temporary data
    std::size_t checkpoint = allocator.get_marker();
    std::cout << "Saved checkpoint at offset " << checkpoint << "\n";
    
    // Allocate some temporary floats
    float* floats = allocator.allocate<float>(7);
    for (int i = 0; i < 7; ++i) {
        floats[i] = i * 0.25f;
    }
    
    std::cout << "Allocated floats: ";
    for (int i = 0; i < 7; ++i) {
        std::cout << floats[i] << " ";
    }
    std::cout << "\n";
    
    std::cout << "Usage after floats: " << allocator.get_used_size() 
              << " / " << allocator.get_total_size() << " bytes\n";
    
    // Allocate some temporary doubles
    double* doubles = allocator.allocate<double>(10);
    for (int i = 0; i < 10; ++i) {
        doubles[i] = i * 1.5;
    }
    
    std::cout << "Allocated doubles: ";
    for (int i = 0; i < 10; ++i) {
        std::cout << doubles[i] << " ";
    }
    std::cout << "\n";
    
    std::cout << "Peak usage: " << allocator.get_used_size() 
              << " / " << allocator.get_total_size() << " bytes\n";
    
    // Free back to checkpoint (removes floats and doubles, keeps integers)
    std::cout << "\nFreeing back to checkpoint...\n";
    allocator.free_to_marker(checkpoint);
    
    // The integers are still valid and accessible
    std::cout << "Integers still valid: ";
    for (int i = 0; i < 7; ++i) {
        std::cout << integers[i] << " ";
    }
    std::cout << "\n";
    
    std::cout << "\n=== Object construction demo ===\n";
    
    // Allocate and manually construct objects
    TestObject* objects = allocator.allocate<TestObject>(3);
    
    // Use placement new to construct objects
    for (int i = 0; i < 3; ++i) {
        new(&objects[i]) TestObject(i + 1, (i + 1) * 2.5);
    }
    
    std::cout << "Objects created, using them:\n";
    for (int i = 0; i < 3; ++i) {
        std::cout << "  Object " << i << ": id=" << objects[i].id 
                  << ", value=" << objects[i].value << "\n";
    }
    
    // Manually destroy objects (important for non-trivial destructors!)
    std::cout << "Manually destroying objects:\n";
    for (int i = 2; i >= 0; --i) {  // Destroy in reverse order
        objects[i].~TestObject();
    }
    
    std::cout << "\n=== Final cleanup ===\n";
    
    // Clear entire allocator
    allocator.clear();
    
    std::cout << "\nKey takeaways:\n";
    std::cout << "- Stack allocators are extremely fast (O(1) allocation)\n";
    std::cout << "- No individual deallocation - use markers for bulk freeing\n";
    std::cout << "- Perfect for temporary allocations with predictable lifetimes\n";
    std::cout << "- Remember to manually destroy non-trivial objects!\n";
    std::cout << "- Excellent cache locality due to linear memory layout\n";
    
    return 0;
}