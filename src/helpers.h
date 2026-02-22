//
// helpers
//

#ifndef HELPERS
#define HELPERS

#include "../src/lib_includes.h"

using std::vector;

void shifter(vector<float> &arr, const float newmin, const float newmax);

bool approx_equal(double a, double b, double tolerance);

absl::CivilDay parse_date(const std::string& s);

double mean(const std::vector<double>& values);

double stddev(const std::vector<double>& values);

template<typename T>
void show_type(T&&) {
    #ifdef __GNUC__
    fmt::println("{}", __PRETTY_FUNCTION__);
    #elif defined(_MSC_VER)
    fmt::println("{}", __FUNCSIG__);
    #else
    fmt::println("Compiler doesn't support type introspection");
    #endif
}

// less typing for static_cast to use uint8_t's as array indices
inline size_t idx(uint8_t i) {
    return static_cast<size_t>(i);
}

#endif