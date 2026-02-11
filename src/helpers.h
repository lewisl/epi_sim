//
// helpers
//

#ifndef HELPERS
#define HELPERS

#include <vector>
#include <string>
#include <absl/time/civil_time.h>

using std::vector;

void shifter(vector<float> &arr, const float newmin, const float newmax);

bool approx_equal(double a, double b, double tolerance);

absl::CivilDay parse_date(const std::string& s);

#endif