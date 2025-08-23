#include <iostream>

int main() {
  int n = 3; // n is an int object
  char c;
  // int *p = &c; // no, illegal
  int *p = &n;
  std::cout << "val: " << n << " addr: " << p << std::endl;
}