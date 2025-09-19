#include <iostream>
#include <memory>

#pragma GCC diagnostic push
// allocator::construct and allocator::destroy are deprecated as of C++ 17
// Must update the CMakeList.txt to compile this example (or builds separately).
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

int main() {
  // Define an allocator for int
  std::allocator<int> allocator;

  // Allocate space for 5 ints
  int *array = allocator.allocate(5);

  // Construct values in the allocated memory
  for (int i = 0; i < 5; ++i) {
    allocator.construct(&array[i], i * 10); // Initializes array[i] to i * 10
  }

  // Print the values
  for (int i = 0; i < 5; ++i) {
    std::cout << array[i] << " ";
  }
  std::cout << std::endl;

  // Destroy the constructed objects
  for (int i = 0; i < 5; ++i) {
    allocator.destroy(&array[i]);
  }

  // Deallocate the memory
  allocator.deallocate(array, 5);

  return 0;
}

#pragma GCC diagnostic pop