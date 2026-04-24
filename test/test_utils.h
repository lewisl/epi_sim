// test_utils.h
#pragma once
#include <iostream>
#include <stdexcept>
#include <string_view>

inline int& test_failure_count() { static int n = 0; return n; }
inline int& test_pass_count()    { static int n = 0; return n; }

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (expr) {                                                          \
            ++test_pass_count();                                             \
        } else {                                                             \
            ++test_failure_count();                                          \
            std::cerr << "[FAIL] " #expr "\n"                               \
                      << "       " __FILE__ ":" << __LINE__ << "\n";        \
        }                                                                    \
    } while (0)

#define REQUIRE(expr)                                                        \
    do {                                                                     \
        if (!(expr)) {                                                       \
            ++test_failure_count();                                          \
            std::cerr << "[FAIL/ABORT] " #expr "\n"                         \
                      << "            " __FILE__ ":" << __LINE__ << "\n";   \
            throw std::logic_error("REQUIRE failed: " #expr);               \
        }                                                                    \
    } while (0)

inline void test_summary() {
    int f = test_failure_count(), p = test_pass_count();
    std::cout << "\n" << (p + f) << " checks: " << p << " passed, " << f << " failed\n";
}
