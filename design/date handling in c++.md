### handling dates in c++ with chrono and fmt

This is complicated so it can do everything. Needed but overkill.  Skip down below to abseil CivilDay, which is just a calendar.

```cpp
#include <chrono>
#include <format>
#include <vector>

using namespace std::chrono;

// Create dates
auto start = year{2024}/January/15;  // or year{2024}/1/15
auto end = year{2024}/March/20;

// Generate range
std::vector<year_month_day> dates;
for (auto d = sys_days{start}; d <= sys_days{end}; d += days{1}) {
    dates.push_back(year_month_day{d});
}

// Convert to string (mm-dd-yyyy)
for (const auto& date : dates) {
    auto s = std::format("{:%m-%d-%Y}", date);
    // or std::format("{:02d}-{:02d}-{:04d}", 
    //     unsigned(date.month()), unsigned(date.day()), int(date.year()));
}
```

### using fmt instead of format

```cpp
#include <chrono>
#include <fmt/chrono.h>
#include <vector>

using namespace std::chrono;

auto start = year{2024}/January/15;
auto end = year{2024}/March/20;

std::vector<year_month_day> dates;
for (auto d = sys_days{start}; d <= sys_days{end}; d += days{1}) {
    dates.push_back(year_month_day{d});
}

// Format with fmt
for (const auto& date : dates) {
    auto s = fmt::format("{:%m-%d-%Y}", date);
    // or any other strftime-style format specifier
}
```

### parsing inputs from json

```cpp
#include <chrono>
#include <string>
#include <cstdio>

using namespace std::chrono;

year_month_day parse_date(const std::string& s) {
    int y, m, d;
    std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
    return year{y}/month{m}/day{d};
}

// Usage with nlohmann/json
nlohmann::json j = /* your json */;
auto date = parse_date(j["day1"].get<std::string>());
```

### using Abseil CivilDay

```cpp
#include <absl/time/civil_time.h>
#include <absl/strings/str_format.h>
#include <vector>

// Create dates
absl::CivilDay start(2020, 1, 1);  // year, month, day
absl::CivilDay end(2020, 3, 20);

// Generate range
std::vector<absl::CivilDay> dates;
for (auto d = start; d <= end; ++d) {  // just increment
    dates.push_back(d);
}

// Convert to string
for (const auto& date : dates) {
    auto s = absl::FormatCivilTime(date);  // gives "2020-01-01"
}

// Add days
auto tomorrow = start + 1;
auto next_week = start + 7;

absl::CivilDay parse_date(const std::string& s) {
    int y, m, d;
    std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
    return absl::CivilDay(y, m, d);
}
```