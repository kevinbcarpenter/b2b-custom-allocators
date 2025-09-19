#include <iostream>
#include <vector>
#include <cstddef>  // for std::size_t

template <typename T>
struct TrackingAllocator {
    using value_type = T;

    TrackingAllocator() = default;

    template <typename U>
    constexpr TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

    // Equality is required for allocators
    bool operator==(const TrackingAllocator&) const { return true; }
    bool operator!=(const TrackingAllocator&) const { return false; }

    T* allocate(std::size_t n) {
        std::cout << "ALLOCATING: " << n << " object(s) of size "
                  << sizeof(T) << " bytes.\n";

        // For our example, we forward to the default allocator
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) {
        std::cout << "DEALLOCATING: " << n << " object(s).\n";
        ::operator delete(p);
    }
};

int main() {
    // Here it is! A vector using our custom allocator.
    std::vector<int, TrackingAllocator<int>> vec;

    std::cout << "--> vec.push_back(1);\n";
    vec.push_back(1);

    std::cout << "\n--> vec.push_back(2);\n";
    vec.push_back(2);

    std::cout << "\n--> vec.push_back(3);\n";
    vec.push_back(3);
}