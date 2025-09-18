#include <memory>

int main() {
  int *p = new int(10);
  delete p; // Two steps: 1) Destruct the int object, 2) Deallocate the memory

  std::unique_ptr<int> ptr = std::make_unique<int>(10);
  // No manual delete needed. Destructor and deallocation happen automatically.
}
