#include <iostream>
#include <vector>
#include <memory>
#include <cstring>

// ===== ARENA ALLOCATOR =====
// Allocates memory sequentially from a large block
// Good for: temporary allocations, same lifetime objects
class ArenaAllocator {
private:
char* memory;
size_t size;
size_t offset;

public:
explicit ArenaAllocator(size_t size) : size(size), offset(0) {
memory = new char[size];
std::cout << "Arena: Allocated " << size << " byte arena\n";
}


~ArenaAllocator() {
    delete[] memory;
    std::cout << "Arena: Freed entire arena\n";
}

// Sequential allocation - just bump the pointer
void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t)) {
    // Align the offset
    size_t aligned_offset = (offset + alignment - 1) & ~(alignment - 1);
    
    if (aligned_offset + bytes > size) {
        std::cout << "Arena: Out of memory!\n";
        return nullptr;
    }
    
    void* ptr = memory + aligned_offset;
    offset = aligned_offset + bytes;
    
    std::cout << "Arena: Allocated " << bytes << " bytes at offset " 
              << aligned_offset << "\n";
    return ptr;
}

// Template helper
template<typename T>
T* allocate(size_t count = 1) {
    return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
}

// Can't deallocate individual objects!
void deallocate(void* ptr) {
    std::cout << "Arena: Individual deallocation not supported!\n";
    // No-op - arena doesn't support individual deallocation
}

// Reset entire arena (bulk deallocation)
void reset() {
    offset = 0;
    std::cout << "Arena: Reset - all allocations freed\n";
}

// Save/restore position (for scoped allocation)
size_t save() const { return offset; }
void restore(size_t saved_offset) {
    offset = saved_offset;
    std::cout << "Arena: Restored to offset " << saved_offset << "\n";
}

size_t getBytesUsed() const { return offset; }
size_t getBytesRemaining() const { return size - offset; }


};

// ===== POOL ALLOCATOR =====
// Allocates fixed-size objects from pre-divided chunks
// Good for: objects of same size, individual deallocation needed
template<typename T>
class PoolAllocator {
private:
struct Block {
alignas(T) char data[sizeof(T)];
Block* next;
};


Block* freeList;
std::vector<std::unique_ptr<char[]>> chunks;
size_t objectsPerChunk;
size_t totalAllocated;
size_t totalDeallocated;

void allocateChunk() {
    auto chunk = std::make_unique<char[]>(objectsPerChunk * sizeof(Block));
    Block* blocks = reinterpret_cast<Block*>(chunk.get());
    
    // Link all blocks in this chunk to the free list
    for (size_t i = 0; i < objectsPerChunk - 1; ++i) {
        blocks[i].next = &blocks[i + 1];
    }
    blocks[objectsPerChunk - 1].next = freeList;
    freeList = blocks;
    
    chunks.push_back(std::move(chunk));
    std::cout << "Pool: Allocated new chunk with " << objectsPerChunk 
              << " blocks of " << sizeof(T) << " bytes each\n";
}


public:
explicit PoolAllocator(size_t objectsPerChunk = 1000)
: freeList(nullptr), objectsPerChunk(objectsPerChunk),
totalAllocated(0), totalDeallocated(0) {
allocateChunk();
}


~PoolAllocator() {
    std::cout << "Pool: Destroyed (allocated=" << totalAllocated 
              << ", deallocated=" << totalDeallocated << ")\n";
}

// Allocate one object of type T
T* allocate() {
    if (!freeList) {
        allocateChunk();
    }
    
    Block* block = freeList;
    freeList = freeList->next;
    totalAllocated++;
    
    std::cout << "Pool: Allocated object #" << totalAllocated << "\n";
    return reinterpret_cast<T*>(block);
}

// Deallocate specific object (returns it to free list)
void deallocate(T* ptr) {
    if (!ptr) return;
    
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
    totalDeallocated++;
    
    std::cout << "Pool: Deallocated object (total deallocated=" 
              << totalDeallocated << ")\n";
}

size_t getChunkCount() const { return chunks.size(); }
size_t getActiveObjects() const { return totalAllocated - totalDeallocated; }


};

// ===== COMPARISON EXAMPLES =====

struct GameObject {
float x, y, z;
int health;
char name[32];


GameObject(float x = 0, float y = 0, float z = 0, int health = 100) 
    : x(x), y(y), z(z), health(health) {
    strcpy(name, "DefaultObject");
}

~GameObject() {
    // Some cleanup logic
}


};

void demonstrateArenaUsage() {
std::cout << "\n=== ARENA ALLOCATOR DEMO ===\n";


ArenaAllocator arena(4096);  // 4KB arena

// Scenario 1: Frame-based allocation (games)
std::cout << "\n--- Frame-based allocation ---\n";

// Frame 1
auto frame1_marker = arena.save();
float* vertices = arena.allocate<float>(100);  // Vertex buffer
int* indices = arena.allocate<int>(50);        // Index buffer
char* strings = arena.allocate<char>(200);     // Temporary strings

if (vertices && indices && strings) {
    std::cout << "Frame 1: Allocated vertex buffer, index buffer, strings\n";
    // Use the data...
}

// End of frame - free everything allocated this frame
arena.restore(frame1_marker);

// Frame 2 - reuse the same memory
GameObject* temp_objects = arena.allocate<GameObject>(10);
if (temp_objects) {
    for (int i = 0; i < 10; ++i) {
        new(&temp_objects[i]) GameObject(i, i*2, i*3, 100);
    }
    std::cout << "Frame 2: Created 10 temporary game objects\n";
}

// Scenario 2: Algorithm scratch space
std::cout << "\n--- Algorithm scratch space ---\n";
arena.reset();  // Clear everything

int* working_array = arena.allocate<int>(1000);
int* temp_array = arena.allocate<int>(1000);

if (working_array && temp_array) {
    std::cout << "Algorithm: Allocated working arrays\n";
    // Perform merge sort or other algorithm...
    // No need to free - arena will be reset or destroyed
}

std::cout << "Arena used: " << arena.getBytesUsed() 
          << "/" << arena.getBytesUsed() + arena.getBytesRemaining() << " bytes\n";


}

void demonstratePoolUsage() {
std::cout << "\n=== POOL ALLOCATOR DEMO ===\n";


PoolAllocator<GameObject> objectPool(100);  // 100 objects per chunk

// Scenario 1: Object lifecycle management
std::cout << "\n--- Object lifecycle management ---\n";

std::vector<GameObject*> activeObjects;

// Create some objects
for (int i = 0; i < 250; ++i) {  // More than one chunk
    GameObject* obj = objectPool.allocate();
    if (obj) {
        new(obj) GameObject(i, i*2, i*3, 100);
        activeObjects.push_back(obj);
    }
}

std::cout << "Created " << activeObjects.size() << " objects across " 
          << objectPool.getChunkCount() << " chunks\n";

// Remove some objects randomly (individual deallocation)
for (size_t i = 0; i < activeObjects.size(); i += 3) {
    GameObject* obj = activeObjects[i];
    obj->~GameObject();  // Call destructor
    objectPool.deallocate(obj);
    activeObjects[i] = nullptr;
}

std::cout << "Active objects remaining: " << objectPool.getActiveObjects() << "\n";

// Create more objects (will reuse deallocated slots)
std::cout << "\n--- Reusing deallocated slots ---\n";
for (int i = 0; i < 50; ++i) {
    GameObject* obj = objectPool.allocate();
    if (obj) {
        new(obj) GameObject(-i, -i*2, -i*3, 50);
        // These objects may reuse previously deallocated memory
    }
}

std::cout << "After allocating 50 more: " << objectPool.getActiveObjects() 
          << " active objects\n";

// Cleanup remaining objects
for (auto* obj : activeObjects) {
    if (obj) {
        obj->~GameObject();
        objectPool.deallocate(obj);
    }
}


}

void performanceComparison() {
std::cout << "\n=== PERFORMANCE CHARACTERISTICS ===\n";


const size_t NUM_OBJECTS = 1000;

// Arena allocation pattern
std::cout << "\n--- Arena Pattern ---\n";
{
    ArenaAllocator arena(NUM_OBJECTS * sizeof(GameObject));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    GameObject* objects = arena.allocate<GameObject>(NUM_OBJECTS);
    for (size_t i = 0; i < NUM_OBJECTS; ++i) {
        new(&objects[i]) GameObject(i, i, i, 100);
    }
    
    // Bulk deallocation
    arena.reset();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Arena: " << duration.count() << " microseconds\n";
    std::cout << "       - Very fast allocation (pointer increment)\n";
    std::cout << "       - Instant bulk deallocation\n";
    std::cout << "       - Perfect cache locality\n";
}

// Pool allocation pattern
std::cout << "\n--- Pool Pattern ---\n";
{
    PoolAllocator<GameObject> pool;
    std::vector<GameObject*> objects;
    objects.reserve(NUM_OBJECTS);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_OBJECTS; ++i) {
        GameObject* obj = pool.allocate();
        new(obj) GameObject(i, i, i, 100);
        objects.push_back(obj);
    }
    
    // Individual deallocation
    for (auto* obj : objects) {
        obj->~GameObject();
        pool.deallocate(obj);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Pool: " << duration.count() << " microseconds\n";
    std::cout << "      - Fast allocation (free list traversal)\n";
    std::cout << "      - Individual deallocation supported\n";
    std::cout << "      - Good cache locality within chunks\n";
}


}

int main() {
std::cout << "ARENA vs POOL ALLOCATOR COMPARISON\n";
std::cout << "===================================\n";


demonstrateArenaUsage();
demonstratePoolUsage();
performanceComparison();

std::cout << "\n=== SUMMARY ===\n";
std::cout << "ARENA: Use for temporary allocations, same lifetime, bulk deallocation\n";
std::cout << "POOL:  Use for fixed-size objects, individual deallocation, varying lifetimes\n";

return 0;


}