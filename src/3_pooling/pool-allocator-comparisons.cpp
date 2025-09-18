#include <iostream>
#include <vector>
#include <memory>

// BAD: Pool allocator without proper chunk tracking (MEMORY LEAK!)
template<typename T>
class LeakyPoolAllocator {
private:
struct Block {
alignas(T) char data[sizeof(T)];
Block* next;
};


Block* freeList;

void allocateChunk() {
    const size_t numBlocks = 1000;
    // This memory is never freed! MEMORY LEAK!
    char* chunk = new char[numBlocks * sizeof(Block)];
    
    Block* blocks = reinterpret_cast<Block*>(chunk);
    for (size_t i = 0; i < numBlocks - 1; ++i) {
        blocks[i].next = &blocks[i + 1];
    }
    blocks[numBlocks - 1].next = freeList;
    freeList = blocks;
    
    // Lost the pointer to 'chunk' - can never delete it!
}


public:
LeakyPoolAllocator() : freeList(nullptr) {
allocateChunk();
}


// Destructor can't free chunks because we don't track them!
~LeakyPoolAllocator() {
    // freeList points into chunks we can't free
    // Memory leak here!
}

T* allocate() {
    if (!freeList) {
        allocateChunk(); // Another chunk we'll never free!
    }
    
    Block* block = freeList;
    freeList = freeList->next;
    return reinterpret_cast<T*>(block);
}

void deallocate(T* ptr) {
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
}


};

// GOOD: Pool allocator with proper chunk tracking
template<typename T>
class ProperPoolAllocator {
private:
struct Block {
alignas(T) char data[sizeof(T)];
Block* next;
};


Block* freeList;
std::vector<std::unique_ptr<char[]>> chunks; // Tracks all chunks for cleanup

void allocateChunk() {
    const size_t numBlocks = 1000;
    
    // Create a new chunk and store it in the vector
    auto chunk = std::make_unique<char[]>(numBlocks * sizeof(Block));
    
    // Set up the free list in this chunk
    Block* blocks = reinterpret_cast<Block*>(chunk.get());
    for (size_t i = 0; i < numBlocks - 1; ++i) {
        blocks[i].next = &blocks[i + 1];
    }
    blocks[numBlocks - 1].next = freeList;
    freeList = blocks;
    
    // Store the chunk for later cleanup
    chunks.push_back(std::move(chunk));
}


public:
ProperPoolAllocator() : freeList(nullptr) {
allocateChunk();
}


// Destructor automatically cleans up all chunks
~ProperPoolAllocator() {
    // chunks vector automatically deletes all unique_ptrs
    // which automatically delete the char arrays
    // No memory leaks!
}

T* allocate() {
    if (!freeList) {
        allocateChunk();
    }
    
    Block* block = freeList;
    freeList = freeList->next;
    return reinterpret_cast<T*>(block);
}

void deallocate(T* ptr) {
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
}

// Debug: Show how many chunks we've allocated
size_t getChunkCount() const {
    return chunks.size();
}


};

// ALTERNATIVE 1: Single large allocation (no expansion)
template<typename T, size_t MaxObjects = 10000>
class FixedPoolAllocator {
private:
struct Block {
alignas(T) char data[sizeof(T)];
Block* next;
};


alignas(Block) char memory[MaxObjects * sizeof(Block)]; // Stack allocated
Block* freeList;


public:
FixedPoolAllocator() {
Block* blocks = reinterpret_cast<Block*>(memory);


    // Initialize free list
    for (size_t i = 0; i < MaxObjects - 1; ++i) {
        blocks[i].next = &blocks[i + 1];
    }
    blocks[MaxObjects - 1].next = nullptr;
    freeList = blocks;
}

T* allocate() {
    if (!freeList) {
        return nullptr; // Pool exhausted
    }
    
    Block* block = freeList;
    freeList = freeList->next;
    return reinterpret_cast<T*>(block);
}

void deallocate(T* ptr) {
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
}


};

// ALTERNATIVE 2: Intrusive linked list of chunks
template<typename T>
class IntrusivePoolAllocator {
private:
struct Block {
alignas(T) char data[sizeof(T)];
Block* next;
};


struct Chunk {
    Chunk* nextChunk;
    size_t numBlocks;
    // Blocks follow immediately after this header
    
    Block* getBlocks() {
        return reinterpret_cast<Block*>(this + 1);
    }
};

Block* freeList;
Chunk* chunkList; // Linked list of chunks instead of vector

void allocateChunk() {
    const size_t numBlocks = 1000;
    const size_t chunkSize = sizeof(Chunk) + numBlocks * sizeof(Block);
    
    // Allocate chunk with header
    char* memory = new char[chunkSize];
    Chunk* chunk = reinterpret_cast<Chunk*>(memory);
    
    // Initialize chunk header
    chunk->nextChunk = chunkList;
    chunk->numBlocks = numBlocks;
    chunkList = chunk;
    
    // Initialize blocks in this chunk
    Block* blocks = chunk->getBlocks();
    for (size_t i = 0; i < numBlocks - 1; ++i) {
        blocks[i].next = &blocks[i + 1];
    }
    blocks[numBlocks - 1].next = freeList;
    freeList = blocks;
}


public:
IntrusivePoolAllocator() : freeList(nullptr), chunkList(nullptr) {
allocateChunk();
}


~IntrusivePoolAllocator() {
    // Clean up all chunks in the linked list
    while (chunkList) {
        Chunk* toDelete = chunkList;
        chunkList = chunkList->nextChunk;
        delete[] reinterpret_cast<char*>(toDelete);
    }
}

T* allocate() {
    if (!freeList) {
        allocateChunk();
    }
    
    Block* block = freeList;
    freeList = freeList->next;
    return reinterpret_cast<T*>(block);
}

void deallocate(T* ptr) {
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
}


};

// Test program to demonstrate the differences
struct TestObject {
int value;
TestObject(int v = 0) : value(v) {}
};

int main() {
std::cout << "=== Pool Allocator Comparison ===" << std::endl;


// Test the proper pool allocator
{
    ProperPoolAllocator<TestObject> pool;
    std::vector<TestObject*> objects;
    
    // Allocate enough objects to trigger multiple chunks
    for (int i = 0; i < 2500; ++i) {
        TestObject* obj = pool.allocate();
        if (obj) {
            new(obj) TestObject(i);
            objects.push_back(obj);
        }
    }
    
    std::cout << "Proper pool allocated " << objects.size() 
              << " objects across " << pool.getChunkCount() << " chunks" << std::endl;
    
    // Clean up
    for (auto* obj : objects) {
        obj->~TestObject();
        pool.deallocate(obj);
    }
    
    std::cout << "All objects deallocated, chunks still tracked for cleanup" << std::endl;
} // Destructor automatically cleans up all chunks here

// Test the fixed pool allocator
{
    FixedPoolAllocator<TestObject, 100> fixedPool;
    std::vector<TestObject*> objects;
    
    // This can only allocate up to 100 objects
    for (int i = 0; i < 150; ++i) {
        TestObject* obj = fixedPool.allocate();
        if (obj) {
            new(obj) TestObject(i);
            objects.push_back(obj);
        } else {
            std::cout << "Fixed pool exhausted after " << objects.size() 
                      << " allocations" << std::endl;
            break;
        }
    }
    
    // Clean up
    for (auto* obj : objects) {
        obj->~TestObject();
        fixedPool.deallocate(obj);
    }
} // No dynamic memory to clean up

// Test the intrusive pool allocator
{
    IntrusivePoolAllocator<TestObject> intrusivePool;
    std::vector<TestObject*> objects;
    
    for (int i = 0; i < 2500; ++i) {
        TestObject* obj = intrusivePool.allocate();
        if (obj) {
            new(obj) TestObject(i);
            objects.push_back(obj);
        }
    }
    
    std::cout << "Intrusive pool allocated " << objects.size() << " objects" << std::endl;
    
    // Clean up
    for (auto* obj : objects) {
        obj->~TestObject();
        intrusivePool.deallocate(obj);
    }
} // Destructor walks chunk linked list for cleanup

std::cout << "\nAll allocators properly cleaned up!" << std::endl;

return 0;


}