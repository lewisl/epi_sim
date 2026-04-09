#pragma once

#include "lib_includes.h"
#include "helpers.h"

/*
Note:  these are not what computer languages call type traits.

Traits of people or agents in the simulation.
Most of these trait structs are used as the types of the vectors (semantically, columns)
in the PopData struct, which is an SOA (struct of arrays) pattern for building
the population table for the simulation.
*/


//
// Compile time classes for traits with values that can't be changed
//
/*
use as:

Status person_status = UNEXPOSED;
person_status = INFECTIOUS;    
person_status = RECOVERED;   

*/


// Agegrp
struct Agegrp {
  uint8_t v{};

  static constexpr std::array<std::string, 6> names{
      "unknown", "age0_19", "age20_39", "age40_59", "age60_79", "age80_up"};

  Agegrp() = default;
  constexpr explicit Agegrp(uint8_t v) noexcept : v(v) {}
  constexpr Agegrp(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  Agegrp(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "unknown") return 0;
    if (name == "age0_19") return 1;
    if (name == "age20_39") return 2;
    if (name == "age40_59") return 3;
    if (name == "age60_79") return 4;
    if (name == "age80_up") return 5;
    return 0;
  }

  std::string show() const { return names[v]; }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Agegrp &) const = default;
};

inline constexpr Agegrp UNKNOWN{0};
inline constexpr Agegrp AGE0_19{1};
inline constexpr Agegrp AGE20_39{2};
inline constexpr Agegrp AGE40_59{3};
inline constexpr Agegrp AGE60_79{4};
inline constexpr Agegrp AGE80_UP{5};

// Status
struct Status {
  uint8_t v{};

  static constexpr std::array<std::string, 5> names{
      "none", "unexposed", "infectious", "recovered", "dead"};

  Status() = default;
  constexpr explicit Status(uint8_t v) noexcept : v(v) {}
  constexpr Status(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  Status(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "none") return 0;
    if (name == "unexposed") return 1;
    if (name == "infectious") return 2;
    if (name == "recovered") return 3;
    if (name == "dead") return 4;
    return 0;
  }

  std::string show() const { return names[v]; }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Status &) const = default;
};

inline constexpr Status NONE{0};
inline constexpr Status UNEXPOSED{1};
inline constexpr Status INFECTIOUS{2};
inline constexpr Status RECOVERED{3};
inline constexpr Status DEAD{4};

// Condition
struct Condition {
  uint8_t v{};

  static constexpr std::array<std::string, 5> names{
      "uninfected", "nil", "mild", "sick", "severe"};

  Condition() = default;
  constexpr explicit Condition(uint8_t v) noexcept : v(v) {}
  constexpr Condition(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  Condition(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "uninfected") return 0;
    if (name == "nil") return 1;
    if (name == "mild") return 2;
    if (name == "sick") return 3;
    if (name == "severe") return 4;
    return 0;
  }

  std::string show() const { return names[v]; }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Condition &) const = default;
};

inline constexpr Condition UNINFECTED{0};
inline constexpr Condition NIL{1};
inline constexpr Condition MILD{2};
inline constexpr Condition SICK{3};
inline constexpr Condition SEVERE{4};

// Progressionmap
struct Progressionmap {
  uint8_t v{};

  static constexpr std::array<std::string, 6> names{
    "ToRecover", "ToNil", "ToMild", "ToSick", "ToSevere", "ToDead"};

  std::string name() const noexcept { return names[v]; }

  
  constexpr explicit Progressionmap(uint8_t v) noexcept : v(v) {}
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Progressionmap &) const = default;
};

namespace Progressmap {
  inline constexpr Progressionmap ToRecover{0};
  inline constexpr Progressionmap ToNil{1};
  inline constexpr Progressionmap ToMild{2};
  inline constexpr Progressionmap ToSick{3};
  inline constexpr Progressionmap ToSevere{4};
  inline constexpr Progressionmap ToDead{5};
}

// Vaxstatus -- compile time
struct Vaxstatus {
    uint8_t v{};

  static constexpr std::array<std::string, 4> names{
      "none", "first", "full", "booster"};

  std::string show() const noexcept { return names[v]; }
    
  constexpr explicit Vaxstatus(uint8_t v) noexcept : v(v) {}  // constructor
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Vaxstatus &) const = default;
};

// use as Vaxstat::none, etc.
namespace Vaxstat {
  inline const Vaxstatus none{0};
  inline const Vaxstatus first{1};
  inline const Vaxstatus full{2};
  inline const Vaxstatus booster{3};
} // namespace Vaxstat

struct Duration {
  uint8_t v{};

  constexpr Duration() noexcept = default;
  explicit constexpr Duration(uint8_t value) noexcept : v(value) {}
  constexpr Duration& operator=(uint8_t value) noexcept { v = value; return *this; }

  std::string show() const {
    return fmt::format("{}", static_cast<unsigned int>(v));
  }

  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Duration&) const noexcept = default;
  constexpr auto operator<=>(const Duration&) const noexcept = default;

  constexpr Duration& operator++() {
    ++v;
    return *this;
  }

  constexpr Duration operator++(int) {
    Duration copy = *this;
    ++(*this);
    return copy;
  }

  constexpr Duration& operator+=(int rhs) {
    v = static_cast<uint8_t>(v + rhs);
    return *this;
  }

  friend constexpr Duration operator+(Duration lhs, int rhs) {
    lhs += rhs;
    return lhs;
  }
};

struct Sickday {
  int16_t v{};

  constexpr Sickday() noexcept = default;
  explicit constexpr Sickday(int16_t value) noexcept : v(value) {}
  constexpr Sickday& operator=(int16_t value) noexcept { v = value; return *this; }

  std::string show() const { return fmt::format("{}", v); }
  constexpr operator int16_t() const noexcept { return v; }
  constexpr bool operator==(const Sickday&) const noexcept = default;
  constexpr auto operator<=>(const Sickday&) const noexcept = default;
};

struct Recovday {
  int16_t v{};

  constexpr Recovday() noexcept = default;
  explicit constexpr Recovday(int16_t value) noexcept : v(value) {}
  constexpr Recovday& operator=(int16_t value) noexcept { v = value; return *this; }

  std::string show() const { return fmt::format("{}", v); }
  constexpr operator int16_t() const noexcept { return v; }
  constexpr bool operator==(const Recovday&) const noexcept = default;
  constexpr auto operator<=>(const Recovday&) const noexcept = default;
};

struct Deadday {
  int16_t v{};

  constexpr Deadday() noexcept = default;
  explicit constexpr Deadday(int16_t value) noexcept : v(value) {}
  constexpr Deadday& operator=(int16_t value) noexcept { v = value; return *this; }

  std::string show() const { return fmt::format("{}", v); }
  constexpr operator int16_t() const noexcept { return v; }
  constexpr bool operator==(const Deadday&) const noexcept = default;
  constexpr auto operator<=>(const Deadday&) const noexcept = default;
};

struct Testday {
  int16_t v{};

  constexpr Testday() noexcept = default;
  explicit constexpr Testday(int16_t value) noexcept : v(value) {}
  constexpr Testday& operator=(int16_t value) noexcept { v = value; return *this; }

  std::string show() const { return fmt::format("{}", v); }
  constexpr operator int16_t() const noexcept { return v; }
  constexpr bool operator==(const Testday&) const noexcept = default;
  constexpr auto operator<=>(const Testday&) const noexcept = default;
};

struct Quarday {
  int16_t v{};

  constexpr Quarday() noexcept = default;
  explicit constexpr Quarday(int16_t value) noexcept : v(value) {}
  constexpr Quarday& operator=(int16_t value) noexcept { v = value; return *this; }

  std::string show() const { return fmt::format("{}", v); }
  constexpr operator int16_t() const noexcept { return v; }
  constexpr bool operator==(const Quarday&) const noexcept = default;
  constexpr auto operator<=>(const Quarday&) const noexcept = default;
};

struct Vaxday {
  int16_t v{};

  constexpr Vaxday() noexcept = default;
  explicit constexpr Vaxday(int16_t value) noexcept : v(value) {}
  constexpr Vaxday& operator=(int16_t value) noexcept { v = value; return *this; }

  std::string show() const { return fmt::format("{}", v); }
  constexpr operator int16_t() const noexcept { return v; }
  constexpr bool operator==(const Vaxday&) const noexcept = default;
  constexpr auto operator<=>(const Vaxday&) const noexcept = default;
};

//
// runtime building of trait classes
//

// Variant -- create instances at runtime
struct Variant {
  uint8_t v{};
  inline static std::vector<std::string> names;

  Variant() = default;
  constexpr explicit Variant(uint8_t v) noexcept : v(v) {}
  constexpr Variant(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  explicit Variant(std::string_view name) {
    if (names.empty()) {
      assert(name == "none" && "First Variant constructed must be \"none\"");
    } else {
      assert(name != "none" && "\"none\" variant already exists");
    }
    names.push_back(std::string{name});
    v = static_cast<uint8_t>(names.size() - 1);
  }

  std::string show() const noexcept {
    if (static_cast<size_t>(v) >= names.size()) return "";
    return names[v];
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Variant &) const = default;
};

struct Vax {
  uint8_t v{};
  inline static std::vector<std::string> names;

  Vax() = default;
  constexpr explicit Vax(uint8_t v) noexcept : v(v) {}
  constexpr Vax(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  explicit Vax(std::string_view name) {
    if (names.empty()) {
      assert(name == "none" && "First Vax constructed must be \"none\"");
    } else {
      assert(name != "none" && "\"none\" vax already exists");
    }
    names.push_back(std::string{name});
    v = static_cast<uint8_t>(names.size() - 1);
  }

  std::string show() const noexcept {
    if (static_cast<size_t>(v) >= names.size()) return "";
    return names[v];
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Vax &) const = default;
};

struct VariantHist {
  std::array<Variant, 16> arr{};
  uint8_t count{};

  void set(Variant variant) {
    if (count < arr.size()) {
      arr[count] = variant;
    } else {
      std::shift_left(arr.begin(), arr.end(), 1);
      arr.back() = variant;
    }
    ++count;
  }

  size_t stored_count() const {
    return std::min<size_t>(count, arr.size());
  }

  Variant latest() const {
    if (count == 0) return Variant{};
    return count >= arr.size() ? arr.back() : arr[zidx(count)];
  }

  std::string show() const {
    const auto entry_count = stored_count();
    if (entry_count == 0) return "";

    std::vector<std::string> rendered;
    rendered.reserve(entry_count);
    for (size_t idx = 0; idx < entry_count; ++idx) {
      rendered.push_back(arr[idx].show());
    }
    return fmt::format("{}", fmt::join(rendered, "|"));
  }
};

struct VaxHist {
  std::array<Vax, 16> arr{};
  uint8_t count{};

  void set(Vax vax) {
    if (count < arr.size()) {
      arr[count] = vax;
    } else {
      std::shift_left(arr.begin(), arr.end(), 1);
      arr.back() = vax;
    }
    ++count;
  }

  size_t stored_count() const {
    return std::min<size_t>(count, arr.size());
  }

  Vax latest() const {
    if (count == 0) return Vax{};
    return count >= arr.size() ? arr.back() : arr[zidx(count)];
  }

  std::string show() const {
    const auto entry_count = stored_count();
    if (entry_count == 0) return "";

    std::vector<std::string> rendered;
    rendered.reserve(entry_count);
    for (size_t idx = 0; idx < entry_count; ++idx) {
      rendered.push_back(arr[idx].show());
    }
    return fmt::format("{}", fmt::join(rendered, "|"));
  }
};

struct SickdayHist {
  std::array<int16_t, 16> arr{};
  uint8_t count{};

  void set(int16_t day) {
    if (count < arr.size()) {
      arr[count] = day;
    } else {
      std::shift_left(arr.begin(), arr.end(), 1);
      arr.back() = day;
    }
    ++count;
  }

  size_t stored_count() const {
    return std::min<size_t>(count, arr.size());
  }

  int16_t latest() const {
    if (count == 0) return 0;
    return count >= arr.size() ? arr.back() : arr[zidx(count)];
  }

  std::string show() const {
    const auto entry_count = stored_count();
    if (entry_count == 0) return "";
    std::vector<int16_t> rendered(arr.begin(), arr.begin() + entry_count);
    return fmt::format("{}", fmt::join(rendered, "|"));
  }
};

struct RecovdayHist {
  std::array<int16_t, 16> arr{};
  uint8_t count{};

  void set(int16_t day) {
    if (count < arr.size()) {
      arr[count] = day;
    } else {
      std::shift_left(arr.begin(), arr.end(), 1);
      arr.back() = day;
    }
    ++count;
  }

  size_t stored_count() const {
    return std::min<size_t>(count, arr.size());
  }

  int16_t latest() const {
    if (count == 0) return 0;
    return count >= arr.size() ? arr.back() : arr[zidx(count)];
  }

  std::string show() const {
    const auto entry_count = stored_count();
    if (entry_count == 0) return "";
    std::vector<int16_t> rendered(arr.begin(), arr.begin() + entry_count);
    return fmt::format("{}", fmt::join(rendered, "|"));
  }
};

struct TestdayHist {
  std::array<int16_t, 16> arr{};
  uint8_t count{};

  void set(int16_t day) {
    if (count < arr.size()) {
      arr[count] = day;
    } else {
      std::shift_left(arr.begin(), arr.end(), 1);
      arr.back() = day;
    }
    ++count;
  }

  size_t stored_count() const {
    return std::min<size_t>(count, arr.size());
  }

  int16_t latest() const {
    if (count == 0) return 0;
    return count >= arr.size() ? arr.back() : arr[zidx(count)];
  }

  std::string show() const {
    const auto entry_count = stored_count();
    if (entry_count == 0) return "";
    std::vector<int16_t> rendered(arr.begin(), arr.begin() + entry_count);
    return fmt::format("{}", fmt::join(rendered, "|"));
  }
};

struct VaxdayHist {
  std::array<int16_t, 16> arr{};
  uint8_t count{};

  void set(int16_t day) {
    if (count < arr.size()) {
      arr[count] = day;
    } else {
      std::shift_left(arr.begin(), arr.end(), 1);
      arr.back() = day;
    }
    ++count;
  }

  size_t stored_count() const {
    return std::min<size_t>(count, arr.size());
  }

  int16_t latest() const {
    if (count == 0) return 0;
    return count >= arr.size() ? arr.back() : arr[zidx(count)];
  }

  std::string show() const {
    const auto entry_count = stored_count();
    if (entry_count == 0) return "";
    std::vector<int16_t> rendered(arr.begin(), arr.begin() + entry_count);
    return fmt::format("{}", fmt::join(rendered, "|"));
  }
};

/* trait_from_string<T>(s) -- converts a string name to a trait value.
   T must provide a static names member and accept a uint8_t value constructor.
   Returns std::nullopt if s is not found in T::names.
   Usage:
     auto ret = trait_from_string<Agegrp>("age20_39");
     if (!ret) { // handle bad input }
     Agegrp ag = *ret;
*/
template<typename T>
std::optional<T> trait_from_string(const std::string& s) {
    auto tolower_str = [](const std::string& str) {
        std::string out = str;
        std::transform(out.begin(), out.end(), out.begin(), ::tolower);  // like functional "apply"
        return out;
    };
    std::string sl = tolower_str(s);
    auto it = std::find_if(T::names.begin(), T::names.end(),
        [&](const std::string& name) { return tolower_str(name) == sl; });
    if (it == T::names.end()) return std::nullopt;    // use nullopt instead of nullptr--because the return object is not a pointer. we could use {} instead
    return T{static_cast<uint8_t>(std::distance(T::names.begin(), it))};
}
