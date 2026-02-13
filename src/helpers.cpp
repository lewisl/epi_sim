//
// helpers    
//

#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include <absl/time/civil_time.h>
#include <absl/strings/str_format.h>
#include <numeric>
#include <cmath>

#include "helpers.h"

using std::vector;


void shifter(vector<float> &arr, const float newmin, const float newmax) {  

  auto minposition = std::min_element(arr.begin(), arr.end());
  auto maxposition = std::max_element(arr.begin(), arr.end());
  const float oldmin = *minposition;
  const float oldmax = *maxposition;

  // Handle edge case: all values are identical
  if (oldmax == oldmin) {
    std::fill(arr.begin(), arr.end(), 1.0f);  // Neutral multiplier
  } else {
    for (auto &element : arr) {
      element = newmin + (newmax - newmin) / (oldmax - oldmin) * (element - oldmin); // we can put this in a function if needed
    }
  }
}

bool approx_equal(double a, double b, double tolerance = 1e-9) {
  return std::abs(a - b) < tolerance;
}

absl::CivilDay parse_date(const std::string& s) {
    int y, m, d;
    std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
    return absl::CivilDay(y, m, d);
}



double mean(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double stddev(const std::vector<double>& values) {
    double m = mean(values);
    double sq_sum = std::accumulate(values.begin(), values.end(), 0.0,
        [m](double acc, double val) {
            return acc + (val - m) * (val - m);
        });
    return std::sqrt(sq_sum / values.size());
}