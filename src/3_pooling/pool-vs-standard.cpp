#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

// 1. Simple Pool Allocator
template <typename T, size_t BlockSize = 4096> class PoolAllocator {
private:
  struct Block {
    alignas(T) char data[sizeof(T)];
    Block *next;
  };

  Block *freeList;
  std::vector<std::unique_ptr<char[]>> chunks;

  void allocateChunk() {
    size_t numBlocks = BlockSize / sizeof(Block);
    auto chunk = std::make_unique<char[]>(numBlocks * sizeof(Block));

    Block *blocks = reinterpret_cast<Block *>(chunk.get());
    for (size_t i = 0; i < numBlocks - 1; ++i) {
      blocks[i].next = &blocks[i + 1];
    }
    blocks[numBlocks - 1].next = freeList;
    freeList = blocks;

    chunks.push_back(std::move(chunk));
  }

public:
  PoolAllocator() : freeList(nullptr) { allocateChunk(); }

  T *allocate() {
    if (!freeList) {
      allocateChunk();
    }

    Block *block = freeList;
    freeList = freeList->next;
    return reinterpret_cast<T *>(block);
  }

  void deallocate(T *ptr) {
    if (!ptr)
      return;

    Block *block = reinterpret_cast<Block *>(ptr);
    block->next = freeList;
    freeList = block;
  }
};

// 2. Linear/Stack Allocator (from previous example, simplified)
class LinearAllocator {
  char *memory;
  size_t size;
  size_t offset;

public:
  explicit LinearAllocator(size_t size) : size(size), offset(0) {
    memory = new char[size];
  }

  ~LinearAllocator() { delete[] memory; }

  template <typename T> T *allocate(size_t count = 1) {
    size_t bytes = sizeof(T) * count;
    if (offset + bytes > size)
      return nullptr;

    T *ptr = reinterpret_cast<T *>(memory + offset);
    offset += bytes;
    return ptr;
  }

  void reset() { offset = 0; }
};

// 3. Ring Buffer Allocator
class RingAllocator {
  char *memory;
  size_t size;
  size_t head;
  size_t tail;

public:
  explicit RingAllocator(size_t size) : size(size), head(0), tail(0) {
    memory = new char[size];
  }

  ~RingAllocator() { delete[] memory; }

  void *allocate(size_t bytes) {
    // Simplified - would need proper wraparound logic
    if (head + bytes <= size) {
      void *ptr = memory + head;
      head += bytes;
      return ptr;
    }
    return nullptr;
  }

  void reset() { head = tail = 0; }
};

// 4. Custom STL Allocator
template <typename T> class CustomAllocator {
  static PoolAllocator<T> pool;

public:
  using value_type = T;

  CustomAllocator() = default;
  template <typename U> CustomAllocator(const CustomAllocator<U> &) {}

  T *allocate(size_t n) {
    if (n == 1) {
      return pool.allocate();
    }
    return static_cast<T *>(::operator new(n * sizeof(T)));
  }

  void deallocate(T *ptr, size_t n) {
    if (n == 1) {
      pool.deallocate(ptr);
    } else {
      ::operator delete(ptr);
    }
  }

  template <typename U> bool operator==(const CustomAllocator<U> &) const {
    return true;
  }

  template <typename U> bool operator!=(const CustomAllocator<U> &) const {
    return false;
  }
};

template <typename T> PoolAllocator<T> CustomAllocator<T>::pool;

// 5. Small Object Allocator (Loki-style)
class SmallObjectAllocator {
  static constexpr size_t MAX_SMALL_OBJECT_SIZE = 256;
  static constexpr size_t ALIGNMENT = alignof(std::max_align_t);

  struct Pool {
    char *memory;
    char *next;
    size_t blockSize;
    size_t numBlocks;
  };

  std::vector<Pool> pools;

public:
  void *allocate(size_t bytes) {
    if (bytes > MAX_SMALL_OBJECT_SIZE) {
      return ::operator new(bytes);
    }

    // Find appropriate pool based on size
    size_t index = (bytes + ALIGNMENT - 1) / ALIGNMENT;
    if (index >= pools.size()) {
      pools.resize(index + 1);
    }

    // Simplified pool allocation logic
    return ::operator new(bytes); // Fallback for demo
  }

  void deallocate(void *ptr, size_t bytes) {
    if (bytes > MAX_SMALL_OBJECT_SIZE) {
      ::operator delete(ptr);
      return;
    }

    // Return to appropriate pool
    ::operator delete(ptr); // Fallback for demo
  }
};

// Performance comparison example
struct TestObject {
  int data[16]; // 64 bytes
  TestObject() { data[0] = 42; }
};

void performanceTest() {
  const size_t NUM_ALLOCATIONS = 100000;

  // Test standard allocator
  auto start = std::chrono::high_resolution_clock::now();

  std::vector<TestObject *> objects;
  objects.reserve(NUM_ALLOCATIONS);

  for (size_t i = 0; i < NUM_ALLOCATIONS; ++i) {
    objects.push_back(new TestObject());
  }

  for (auto *obj : objects) {
    delete obj;
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Standard allocator: " << duration.count() << " microseconds"
            << std::endl;

  // Test pool allocator
  start = std::chrono::high_resolution_clock::now();

  PoolAllocator<TestObject> pool;
  objects.clear();

  for (size_t i = 0; i < NUM_ALLOCATIONS; ++i) {
    TestObject *obj = pool.allocate();
    new (obj) TestObject(); // Placement new
    objects.push_back(obj);
  }

  for (auto *obj : objects) {
    obj->~TestObject(); // Manual destructor call
    pool.deallocate(obj);
  }

  end = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Pool allocator: " << duration.count() << " microseconds"
            << std::endl;
}

int main() {
  std::cout << "=== C++ Allocator Examples ===" << std::endl;

  // Demo pool allocator
  PoolAllocator<int> intPool;
  int *nums[10];
  for (int i = 0; i < 10; ++i) {
    nums[i] = intPool.allocate();
    *nums[i] = i * i;
  }

  std::cout << "Pool allocated numbers: ";
  for (int i = 0; i < 10; ++i) {
    std::cout << *nums[i] << " ";
    intPool.deallocate(nums[i]);
  }
  std::cout << std::endl;

  // Demo linear allocator
  LinearAllocator linear(1024);
  int *arr = linear.allocate<int>(5);
  for (int i = 0; i < 5; ++i) {
    arr[i] = i + 100;
  }

  std::cout << "Linear allocated array: ";
  for (int i = 0; i < 5; ++i) {
    std::cout << arr[i] << " ";
  }
  std::cout << std::endl;

  // Demo custom STL allocator
  std::vector<int, CustomAllocator<int>> customVec;
  customVec.push_back(42);
  customVec.push_back(84);
  std::cout << "Custom allocator vector: " << customVec[0] << ", "
            << customVec[1] << std::endl;

  // Performance comparison
  std::cout << "\n=== Performance Comparison ===" << std::endl;
  performanceTest();

  return 0;
}