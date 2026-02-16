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
    std::cout << __PRETTY_FUNCTION__ << '\n';
    #elif defined(_MSC_VER)
    std::cout << __FUNCSIG__ << '\n';
    #else
    std::cout << "Compiler doesn't support type introspection\n";
    #endif
}

#endif