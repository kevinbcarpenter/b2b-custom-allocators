#include <memory_resource>
#include <vector>
#include <iostream>

int main() {
    // Create a buffer with a fixed size
    std::byte buffer[1024];
    
    // Create a monotonic buffer resource using the buffer
    std::pmr::monotonic_buffer_resource pool(buffer, sizeof(buffer));

    // Create a pmr::vector using the monotonic buffer resource
    std::pmr::vector<int> vec(&pool);
    
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    // Display elements
    for (int i : vec) {
        std::cout << i << " ";
    }

    return 0;
}