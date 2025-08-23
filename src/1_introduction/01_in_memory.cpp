#include <iostream>
#include <string>

auto addressOut(const std::string &s) -> const void * {
  const char *addr = s.data();
  return static_cast<const void *>(addr);
}

int main() {
  std::string a = "Meeting";
  std::string b = "C++";
  std::string c = "2024";

  // Print the address
  std::cout << a << ": " << addressOut(a) << std::endl;
  std::cout << b << ": " << addressOut(b) << std::endl;
  std::cout << c << ": " << addressOut(c) << std::endl;
  std::cout << a << " " << b << " " << c << std::endl;
}