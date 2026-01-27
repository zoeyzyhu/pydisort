//! This program tests the clone method of the DisortOptionsImpl class.

#include <disort/disort.hpp>

int main() {
  // Create an instance of DisortOptionsImpl
  auto original = disort::DisortOptionsImpl::create();

  // Set some options
  original->header("Test Header");

  // Clone the original instance
  auto clone = original->clone();

  // Report both original and clone to verify they are the same
  original->report(std::cout);
  clone->report(std::cout);

  // edit the clone to verify independence
  clone->header("Modified Clone Header");

  std::cout << "\nAfter modifying the clone:\n";
  original->report(std::cout);
  clone->report(std::cout);

  return 0;
}
