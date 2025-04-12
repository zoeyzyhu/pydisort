#pragma once

// C/C++
#include <cstring>
#include <string>
#include <vector>

namespace disort {
//! split a string to a vector
template <typename A>
std::vector<A> Vectorize(const char *cstr, const char *delimiter = " ") {
  std::vector<A> arr;
  char str[1028], *p;
  snprintf(str, sizeof(str), "%s", cstr);
  p = std::strtok(str, delimiter);
  while (p != NULL) {
    arr.push_back(static_cast<A>(std::stof(p)));
    p = std::strtok(NULL, delimiter);
  }
  return arr;
}

template <>
std::vector<std::string> Vectorize(const char *cstr, const char *delimiter);
}  // namespace disort
