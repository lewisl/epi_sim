#pragma once

#include "absl/container/flat_hash_map.h"
#include <absl/strings/str_format.h>
#include <absl/time/civil_time.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <csv2/reader.hpp>
#include <filesystem>
#include <fmt/base.h>
#include <fmt/format.h> // only get what I use: about 12k in the executable!
#include <fmt/ranges.h> // for printing containers like vector
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp> // amazing for parsing complex files (maybe not for high speed web services)
#include <numeric>
#include <random> // for std::gamma_distribution
#include <ranges>
#include <string>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>