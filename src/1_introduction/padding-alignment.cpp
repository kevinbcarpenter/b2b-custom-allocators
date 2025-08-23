#include <iostream>
#include <string>

struct WhatsMySize {
  char a; // 1 byte
  int b;  // 4 bytes
  char x;
  std::string c;
};

int main() {
  WhatsMySize e;
  std::cout << sizeof(e) << "\n";
}