//
// helpers
//

#ifndef HELPERS
#define HELPERS

#include <vector>
#include <algorithm>      // used by the definition of shifter function in helpers.cpp
#include <cmath>

using std::vector;

void shifter(vector<float> &arr, const float newmin, const float newmax);

bool approx_equal(double a, double b, double tolerance);

#endif