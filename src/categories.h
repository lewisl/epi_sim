#ifndef CATEGORIES_H
#define CATEGORIES_H

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

using std::vector;
using std::string;

// struct to hold category data
struct CATEGORY{
  vector<uint8_t> num;
  vector<string> name;

  // Get name from index (1-based)
  string get_name(uint8_t idx) const {
      if (idx == 0 || idx > name.size()) {
          throw std::out_of_range("Index out of bounds");
      }
      return name[idx - 1];  // human readable nums start at 1; vector indexing starts at 0
  }

  // Get index from name (returns 1-based index)
  uint8_t get_idx(const string& str) const {
      auto it = std::find(name.begin(), name.end(), str);
      if (it == name.end()) {
          throw std::runtime_error("Element not found in category");
      }
      return static_cast<uint8_t>(it - name.begin() + 1);  // vector indexing is 0-based; human readable starts at 1
  }

  // Functor: allows using category struct name as function to get index from name
  uint8_t operator()(const string& str) const {
      return get_idx(str);
  }
};

// category struct instances for major categories
extern const CATEGORY STATUS;

extern const CATEGORY CONDITION;

extern const CATEGORY AGEGRP;

extern const CATEGORY VACCINE;

// category constants for comparisons, filters, assignments
// status
extern const uint8_t UNEXPOSED;
extern const uint8_t INFECTIOUS;
extern const uint8_t RECOVERED;
extern const uint8_t DEAD;

// condition
extern const uint8_t UNINFECTED;
extern const uint8_t NIL;
extern const uint8_t MILD;
extern const uint8_t SICK;
extern const uint8_t SEVERE;



// test function
void run_category_tests();


#endif