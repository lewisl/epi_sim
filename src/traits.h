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
person_status = INFECTIOUS;    // fine
person_status = RECOVERED;   // fine

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

  std::string name() const noexcept { return names[v]; }
    
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

template<typename T, typename Tag>
struct PrimitiveCol {
  using value_type = T;
  using tag_type = Tag;

  T v{};

  constexpr PrimitiveCol() noexcept = default;
  constexpr PrimitiveCol(const PrimitiveCol&) noexcept = default;
  constexpr PrimitiveCol& operator=(const PrimitiveCol&) noexcept = default;
  constexpr PrimitiveCol(PrimitiveCol&&) noexcept = default;
  constexpr PrimitiveCol& operator=(PrimitiveCol&&) noexcept = default;

  explicit constexpr PrimitiveCol(T value) noexcept : v(value) {}

  template<std::integral U>
    requires (!std::same_as<std::remove_cvref_t<U>, PrimitiveCol> &&
              !std::same_as<std::remove_cvref_t<U>, PrimitiveCol<T, Tag>>)
  explicit constexpr PrimitiveCol(U value) : v(checked_cast(value)) {}

  template<std::integral U>
    requires (!std::same_as<std::remove_cvref_t<U>, PrimitiveCol<T, Tag>>)
  constexpr PrimitiveCol& operator=(U value) {
    v = checked_cast(value);
    return *this;
  }

  std::string show() const {
    if constexpr (std::same_as<T, uint8_t>) {
      return fmt::format("{}", static_cast<unsigned int>(v));
    } else {
      return fmt::format("{}", v);
    }
  }

  constexpr operator T() const noexcept { return v; }
  constexpr bool operator==(const PrimitiveCol&) const noexcept = default;
  constexpr auto operator<=>(const PrimitiveCol&) const noexcept = default;

 protected:
  template<std::integral U>
  static constexpr T checked_cast(U value) {
    if (!std::in_range<T>(value)) {
      throw std::out_of_range(
          fmt::format("Value {} is out of range for {}", value, typeid(T).name()));
    }
    return static_cast<T>(value);
  }
};

struct DurationTag;
using DurationBase = PrimitiveCol<uint8_t, DurationTag>;

struct Duration : DurationBase {
  using DurationBase::DurationBase;
  using DurationBase::operator=;

  constexpr Duration& operator++() {
    v = checked_cast(static_cast<int>(v) + 1);
    return *this;
  }

  constexpr Duration operator++(int) {
    Duration copy = *this;
    ++(*this);
    return copy;
  }

  template<std::integral U>
  constexpr Duration& operator+=(U rhs) {
    v = checked_cast(static_cast<long long>(v) + static_cast<long long>(rhs));
    return *this;
  }

  template<std::integral U>
  friend constexpr Duration operator+(Duration lhs, U rhs) {
    lhs += rhs;
    return lhs;
  }
};

using Sickday = PrimitiveCol<int16_t, struct SickdayTag>;
using Recovday = PrimitiveCol<int16_t, struct RecovdayTag>;
using Deadday = PrimitiveCol<int16_t, struct DeaddayTag>;
using Testday = PrimitiveCol<int16_t, struct TestdayTag>;
using Quarday = PrimitiveCol<int16_t, struct QuardayTag>;
using Vaxday = PrimitiveCol<int16_t, struct VaxdayTag>;

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

// print any vector of trait class instances (types with .show() method)
template<typename T>
  requires requires(const T& t) { { t.show() } -> std::convertible_to<std::string>; }
void print_trait_vector(const vector<T>& vec) {
  for (size_t i = 0; i < vec.size(); ++i) {
    fmt::print("{:>2}: {}\n", i, vec[i].show());
  }
}

// enable fmt to format trait classes
template <typename T>
concept TraitType = std::same_as<T, Status> || std::same_as<T, Agegrp> ||
                    std::same_as<T, Condition> ||
                    std::same_as<T, Progressionmap> ||
                    std::same_as<T, Vaxstatus>;
template<TraitType T>
struct fmt::formatter<T> : fmt::formatter<uint8_t> {
    auto format(const T& val, fmt::format_context& ctx) const {
        return fmt::formatter<uint8_t>::format(static_cast<uint8_t>(val), ctx);
    }
};

template <typename T>
concept PrimitiveColType = requires(const T& value) {
  typename T::value_type;
  typename T::tag_type;
  { value.v };
};

template<PrimitiveColType T>
struct fmt::formatter<T> : fmt::formatter<std::conditional_t<std::same_as<typename T::value_type, uint8_t>,
                                                              unsigned int,
                                                              typename T::value_type>> {
  auto format(const T& val, fmt::format_context& ctx) const {
    if constexpr (std::same_as<typename T::value_type, uint8_t>) {
      return fmt::formatter<unsigned int>::format(static_cast<unsigned int>(val.v), ctx);
    } else {
      return fmt::formatter<typename T::value_type>::format(val.v, ctx);
    }
  }
};

/* trait_from_string<T>(s) -- converts a string name to a trait value.
   T must be a TraitType (Agegrp, Status, Condition, etc.).
   Returns std::nullopt if s is not found in T::names.
   Usage:
     auto ret = trait_from_string<Agegrp>("age20_39");
     if (!ret) { // handle bad input }
     Agegrp ag = *ret;
*/
template<TraitType T>
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