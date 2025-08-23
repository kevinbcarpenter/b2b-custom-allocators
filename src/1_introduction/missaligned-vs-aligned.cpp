#include <cstddef> // For std::size_t
#include <iostream>

// Structure with members ordered to maximize padding
struct Misaligned {
  char a;   // 1 byte
  int b;    // 4 bytes
  char c;   // 1 byte
  double d; // 8 bytes
};

// Structure with members ordered to minimize padding
struct Aligned {
  double d; // 8 bytes
  int b;    // 4 bytes
  char a;   // 1 byte
  char c;   // 1 byte
};

int main() {
  // Print the size of each struct
  std::cout << "Size of Misaligned struct: " << sizeof(Misaligned)
            << " bytes\n";
  std::cout << "Size of Aligned struct: " << sizeof(Aligned) << " bytes\n";

  // Print the offsets of each member in Misaligned
  std::cout << "\nOffsets in Misaligned struct:\n";
  std::cout << "a: " << offsetof(Misaligned, a) << " bytes\n";
  std::cout << "b: " << offsetof(Misaligned, b) << " bytes\n";
  std::cout << "c: " << offsetof(Misaligned, c) << " bytes\n";
  std::cout << "d: " << offsetof(Misaligned, d) << " bytes\n";

  // Print the offsets of each member in Aligned
  std::cout << "\nOffsets in Aligned struct:\n";
  std::cout << "d: " << offsetof(Aligned, d) << " bytes\n";
  std::cout << "b: " << offsetof(Aligned, b) << " bytes\n";
  std::cout << "a: " << offsetof(Aligned, a) << " bytes\n";
  std::cout << "c: " << offsetof(Aligned, c) << " bytes\n";

  return 0;
}