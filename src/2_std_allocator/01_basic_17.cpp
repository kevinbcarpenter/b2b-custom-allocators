#include <iostream>
#include <memory> // for std::allocator

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