#include <iostream>
#include <memory_resource>
#include <vector>

int main() {
  // Create a pool resource for efficient small object allocations
  std::byte buffer[1024];
  std::pmr::monotonic_buffer_resource buffer_resource(buffer, sizeof(buffer));

  // Create a pmr::vector using the pool resource
  std::pmr::vector<int> vec(&buffer_resource);

  for (int i = 0; i < 100; ++i) {
    vec.push_back(i);
  }

  std::cout << "Vector contents: ";
  for (int i : vec) {
    std::cout << i << " ";
  }

  return 0;
}