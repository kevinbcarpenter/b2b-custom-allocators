#include <iostream>
#include <memory> // for std::allocator, std::construct_at, std::destroy_at

int main() {
  std::allocator<int> allocator;      // Define an allocator for int
  int *array = allocator.allocate(5); // Allocate space for 5 ints

  for (int i = 0; i < 5; ++i) { // Construct values in the allocated memory
    std::construct_at(&array[i], i * 10); // Initializes array[i] to i * 10
  }

  for (int i = 0; i < 5; ++i) { std::cout << array[i] << " "; }

  // Destroy the constructed objects then deallocate
  for (int i = 0; i < 5; ++i) { std::destroy_at(&array[i]); }

  allocator.deallocate(array, 5);
}