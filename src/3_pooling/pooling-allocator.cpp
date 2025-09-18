#include <cstddef>
#include <iostream>
#include <list>
#include <memory>
#include <vector>

template <typename T> class PoolAllocator {
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // Constructor and Destructor
  PoolAllocator(size_type poolSize = 1024)
      : poolSize_(poolSize), poolStart_(nullptr), poolEnd_(nullptr) {
    allocatePool(); // Initialize the memory pool
  }

  ~PoolAllocator() {
    clearPool(); // Free all allocated memory
  }

  // Allocate memory for `n` elements
  pointer allocate(size_type n) {
    if (freeList_.empty() || n > availableBlocks()) {
      throw std::bad_alloc(); // Not enough memory
    }

    pointer result = freeList_.front();
    for (size_type i = 0; i < n; ++i) {
      freeList_.pop_front(); // Remove blocks from the free list
    }
    return result;
  }

  // Deallocate memory for `n` elements
  void deallocate(pointer p, size_type n) {
    for (size_type i = 0; i < n; ++i) {
      freeList_.push_back(p + i); // Return each block to the free list
    }
  }

  // Construct an object in allocated memory
  template <typename U, typename... Args> void construct(U *p, Args &&...args) {
    new (p) U(std::forward<Args>(args)...); // Placement new
  }

  // Destroy an object in allocated memory
  template <typename U> void destroy(U *p) { p->~U(); }

  // Equality operators
  bool operator==(const PoolAllocator &) const { return true; }
  bool operator!=(const PoolAllocator &) const { return false; }

private:
  size_type poolSize_;          // Number of blocks in the pool
  pointer poolStart_;           // Start of the memory pool
  pointer poolEnd_;             // End of the memory pool
  std::list<pointer> freeList_; // Free list of available blocks

  void allocatePool() {
    poolStart_ =
        static_cast<pointer>(::operator new(poolSize_ * sizeof(value_type)));
    poolEnd_ = poolStart_ + poolSize_;

    // Populate the free list with individual blocks
    for (pointer p = poolStart_; p != poolEnd_; ++p) {
      freeList_.push_back(p);
    }
  }

  void clearPool() {
    ::operator delete(poolStart_); // Free the entire pool
    freeList_.clear();
  }

  size_type availableBlocks() const {
    return static_cast<size_type>(
        std::distance(freeList_.begin(), freeList_.end()));
  }
};

int main() {
  try {
    std::vector<int, PoolAllocator<int>> vec(PoolAllocator<int>(10));

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    for (size_t i = 0; i < vec.size(); ++i) {
      std::cout << "Element " << i + 1 << ": " << vec[i]
                << " (Address: " << &vec[i] << ")" << std::endl;
    }
  } catch (const std::bad_alloc &) {
    std::cerr << "Memory allocation failed!" << std::endl;
  }

  return 0;
}