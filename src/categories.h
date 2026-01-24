#ifndef CATEGORIES_H
#define CATEGORIES_H

#include <cstdint>
#include <string>


namespace Status {
    enum Value : uint8_t { none = 0, unexposed, infectious, recovered, dead };

    // Index to String: O(1) array lookup
    inline const char* to_str(uint8_t v) {
        // static const array lives in data segment, no allocation at runtime
        static const char* names[] = {"none", "unexposed", "infectious", "recovered", "dead"};
        return (v < 5) ? names[v] : "unknown";
    }

    // String to Index: Fast branching (better than a hash map for N < 15)
    inline uint8_t from_str(const std::string &s) {
        if (s == "unexposed")  return unexposed;
        if (s == "infectious") return infectious;
        if (s == "recovered")  return recovered;
        if (s == "dead") return dead;
        if (s == "none") return none;
        return none;
    }

    inline auto format_as(Value v) {
        return to_str(v);
    }

    } // namespace Status


    namespace Condition {
    enum Value : uint8_t { uninfected = 0, nil, mild, sick, severe };

    // Index to String: O(1) array lookup
    inline const char* to_str(uint8_t v) {
        // static const array lives in data segment, no allocation at runtime
        static const char* names[] = {"uninfected", "nil", "mild", "sick", "severe"};
        return (v < 5) ? names[v] : "unknown";
    }

    // String to Index: Fast branching (better than a hash map for N < 15)
    inline uint8_t from_str(const std::string &s) {
        if (s == "uninfected")  return uninfected;
        if (s == "nil") return nil;
        if (s == "mild")  return mild;
        if (s == "sick") return sick;
        if (s == "severe") return severe;
        return uninfected;
    }

    inline auto format_as(Value v) {
      return to_str(v);
    }
}


namespace Agegrp {
    enum Value : uint8_t { unknown = 0, age0_19, age20_39, age40_59, age60_79, age80_up };

    // Index to String: O(1) array lookup
    inline const char* to_str(uint8_t v) {
        // static const array lives in data segment, no allocation at runtime
        static const char* names[] = {"unknown", "age0_19", "age20_39", "age40_59", "age60_79", "age80_up"};
        return (v < 6) ? names[v] : "unknown";
    }

    // String to Index: Fast branching (better than a hash map for N < 15)
    inline uint8_t from_str(const std::string &s) {
        if (s == "age0_19")  return age0_19;
        if (s == "age20_39") return age20_39;
        if (s == "age40_59") return age40_59;
        if (s == "age60_79") return age60_79;
        if (s == "age80_up") return age80_up;
        if (s == "unknown")  return unknown;
        return unknown;
    }

    inline auto format_as(Value v) {
        return to_str(v);
    }
}

// make this a RuntimeEnum because it is based on user input
// namespace Vaccine {
//     enum Value : uint8_t { none = 0, Pfizer, Moderna, JnJ };

//     // Index to String: O(1) array lookup
//     inline const char* to_str(uint8_t v) {
//         // static const array lives in data segment, no allocation at runtime
//         static const char* names[] = {"none", "Pfizer", "Moderna", "J & J"};
//         return (v < 4) ? names[v] : "unknown";
//     }

//     // String to Index: Fast branching (better than a hash map for N < 15)
//     inline uint8_t from_str(const std::string &s) {
//         if (s == "none")    return none;
//         if (s == "Pfizer")  return Pfizer;
//         if (s == "Moderna") return Moderna;
//         if (s == "J & J")   return JnJ;
//         if (s == "JnJ")     return JnJ;  // accept both spellings
//         return none;
//     }
// }


// test function
void run_category_tests();


#endif