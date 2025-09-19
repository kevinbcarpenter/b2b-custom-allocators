#include <iostream>

int main() {
  int* yr = new int(2025);
  std::cout << *yr << std::endl;
  delete yr;
  return 0;
}