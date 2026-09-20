#pragma once

#include "helpers.h"
#include <magic_enum/magic_enum.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <magic_enum/magic_enum.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <fmt/base.h>
#include <fmt/format.h> // only get what I use: about 12k in the executable!
#include <fmt/ranges.h> // for printing containers like vector
#include <fmt/ostream.h> // to use ostream file handles and << >> operators
#include <vector>
#include <string>

/*
Note:  these are not what computer languages call type traits.  These are 
       people's traits (or agents, per the epidemiology literature) in the simulation.

Most of these trait structs are used as the types of the vectors (semantically, columns)
in the PopData struct, which is an SOA (struct of arrays) pattern for building
the population table for the simulation.

The values of Traits are used in the Histories vectors for some traits important to track during the 
simulation and plot or serialize after the simulation.
*/


/*
Compile time classes for traits with values that can't be changed

use as:

  Status person_status = UNEXPOSED;
  person_status = INFECTIOUS;    
  person_status = RECOVERED;   
*/


// Agegrp: enum metadata with one-byte PopData storage.
// Agegrp: enum metadata with one-byte PopData storage.
struct Agegrp {
  enum class Enum : uint8_t {
    unknown = 0, age0_19 = 1, age20_39 = 2, age40_59 = 3, age60_79 = 4, age80_up = 5
  };

  uint8_t v{};
  static constexpr auto names = magic_enum::enum_names<Enum>();

  Agegrp() = default;
  constexpr explicit Agegrp(uint8_t v) noexcept : v(v) {}
  constexpr Agegrp(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  constexpr explicit Agegrp(Enum value) noexcept : v(std::to_underlying(value)) {}
  Agegrp(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    return std::to_underlying(
        magic_enum::enum_cast<Enum>(name, magic_enum::case_insensitive)
            .value_or(Enum::unknown));
  }

  std::string show() const {
    return std::string{magic_enum::enum_name(static_cast<Enum>(v))};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Agegrp&) const = default;
};

static_assert(sizeof(Agegrp) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Agegrp>);

// constants for Agegrp instances used in PopData and Series
inline constexpr Agegrp UNKNOWN{Agegrp::Enum::unknown};
inline constexpr Agegrp AGE0_19{Agegrp::Enum::age0_19};
inline constexpr Agegrp AGE20_39{Agegrp::Enum::age20_39};
inline constexpr Agegrp AGE40_59{Agegrp::Enum::age40_59};
inline constexpr Agegrp AGE60_79{Agegrp::Enum::age60_79};
inline constexpr Agegrp AGE80_UP{Agegrp::Enum::age80_up};

static_assert(UNKNOWN.v == 0 && AGE0_19.v == 1 && AGE20_39.v == 2
              && AGE40_59.v == 3 && AGE60_79.v == 4 && AGE80_UP.v == 5);

// Status: enum metadata with one-byte PopData storage.
// Status: enum metadata with one-byte PopData storage.
struct Status {
  enum class Enum : uint8_t {
    none = 0, unexposed = 1, infectious = 2, recovered = 3, dead = 4
  };

  uint8_t v{};
  static constexpr auto names = magic_enum::enum_names<Enum>();

  Status() = default;
  constexpr explicit Status(uint8_t v) noexcept : v(v) {}
  constexpr Status(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  constexpr explicit Status(Enum value) noexcept : v(std::to_underlying(value)) {}
  Status(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    return std::to_underlying(
        magic_enum::enum_cast<Enum>(name, magic_enum::case_insensitive)
            .value_or(Enum::none));
  }

  std::string show() const {
    return std::string{magic_enum::enum_name(static_cast<Enum>(v))};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Status&) const = default;
};

static_assert(sizeof(Status) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Status>);

// constants for Status instances used in PopData and Series
inline constexpr Status NONE{Status::Enum::none};
inline constexpr Status UNEXPOSED{Status::Enum::unexposed};
inline constexpr Status INFECTIOUS{Status::Enum::infectious};
inline constexpr Status RECOVERED{Status::Enum::recovered};
inline constexpr Status DEAD{Status::Enum::dead};

static_assert(NONE.v == 0 && UNEXPOSED.v == 1 && INFECTIOUS.v == 2
              && RECOVERED.v == 3 && DEAD.v == 4);

// Condition: enum metadata with one-byte PopData storage.
// Condition: enum metadata with one-byte PopData storage.
struct Condition {
  enum class Enum : uint8_t {
    uninfected = 0, nil = 1, mild = 2, sick = 3, severe = 4
  };

  uint8_t v{};
  static constexpr auto names = magic_enum::enum_names<Enum>();

  Condition() = default;
  constexpr explicit Condition(uint8_t v) noexcept : v(v) {}
  constexpr Condition(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  constexpr explicit Condition(Enum value) noexcept : v(std::to_underlying(value)) {}
  Condition(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    return std::to_underlying(
        magic_enum::enum_cast<Enum>(name, magic_enum::case_insensitive)
            .value_or(Enum::uninfected));
  }

  std::string show() const {
    return std::string{magic_enum::enum_name(static_cast<Enum>(v))};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Condition&) const = default;
};

static_assert(sizeof(Condition) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Condition>);

// constants for Condition instances used in PopData
inline constexpr Condition UNINFECTED{Condition::Enum::uninfected};
inline constexpr Condition NIL{Condition::Enum::nil};
inline constexpr Condition MILD{Condition::Enum::mild};
inline constexpr Condition SICK{Condition::Enum::sick};
inline constexpr Condition SEVERE{Condition::Enum::severe};

static_assert(UNINFECTED.v == 0 && NIL.v == 1 && MILD.v == 2
              && SICK.v == 3 && SEVERE.v == 4);

// Vaxstatus: enum metadata with one-byte PopData storage.
struct Vaxstatus {
  enum class Enum : uint8_t {
    none = 0, first = 1, full = 2, booster = 3, exhausted = 4
  };

  uint8_t v{};
  static constexpr auto names = magic_enum::enum_names<Enum>();

  constexpr explicit Vaxstatus(uint8_t v) noexcept : v(v) {}
  constexpr explicit Vaxstatus(Enum value) noexcept : v(std::to_underlying(value)) {}

  std::string show() const noexcept {
    return std::string{magic_enum::enum_name(static_cast<Enum>(v))};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Vaxstatus&) const = default;
};

static_assert(sizeof(Vaxstatus) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Vaxstatus>);

static_assert(sizeof(Vaxstatus) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Vaxstatus>);

namespace Vaxstat {
  inline constexpr Vaxstatus none{Vaxstatus::Enum::none};
  inline constexpr Vaxstatus first{Vaxstatus::Enum::first};
  inline constexpr Vaxstatus full{Vaxstatus::Enum::full};
  inline constexpr Vaxstatus booster{Vaxstatus::Enum::booster};
  inline constexpr Vaxstatus exhausted{Vaxstatus::Enum::exhausted};
} // namespace Vaxstat

static_assert(Vaxstat::none.v == 0 && Vaxstat::first.v == 1
              && Vaxstat::full.v == 2 && Vaxstat::booster.v == 3);

// All six progression outcomes are valid zero-based parameter indices.
enum class Progressionmap : uint8_t {
  ToRecover = 0, ToNil = 1, ToMild = 2, ToSick = 3, ToSevere = 4, ToDead = 5
};

namespace Progressmap {
  inline constexpr auto ToRecover = Progressionmap::ToRecover;
  inline constexpr auto ToNil = Progressionmap::ToNil;
  inline constexpr auto ToMild = Progressionmap::ToMild;
  inline constexpr auto ToSick = Progressionmap::ToSick;
  inline constexpr auto ToSevere = Progressionmap::ToSevere;
  inline constexpr auto ToDead = Progressionmap::ToDead;
}

// do_progression assigns outcomes 1..4 directly to Condition.
static_assert(std::to_underlying(Progressmap::ToRecover) == 0);
static_assert(std::to_underlying(Progressmap::ToNil) == NIL.v);
static_assert(std::to_underlying(Progressmap::ToMild) == MILD.v);
static_assert(std::to_underlying(Progressmap::ToSick) == SICK.v);
static_assert(std::to_underlying(Progressmap::ToSevere) == SEVERE.v);
static_assert(std::to_underlying(Progressmap::ToDead) == 5);

// length of time being infected, per each infection incidence
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

struct Ring {
  uint8_t v{};
  inline static std::vector<std::string> names;

  constexpr Ring() noexcept = default;
  explicit constexpr Ring(uint8_t value) noexcept : v(value) {}
  explicit Ring(std::string_view name) {
    if (names.empty()) {
      names.emplace_back("");  // index 0 reserved as unused sentinel
    }
    assert(!name.empty() && "Ring name must not be empty");
    names.emplace_back(std::string{name});
    v = static_cast<uint8_t>(names.size() - 1);
  }
  constexpr Ring& operator=(uint8_t value) noexcept { v = value; return *this; }

  std::string show() const {
    if (static_cast<size_t>(v) < names.size() && !names[v].empty()) {
      return names[v];
    }
    return fmt::format("{}", static_cast<unsigned int>(v));
  }

  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Ring&) const noexcept = default;
  constexpr auto operator<=>(const Ring&) const noexcept = default;
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
  uint8_t v{};  // will take on values 0..number of variants:  effectively 1-indexed
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

struct SDCase {
  uint8_t v{};
  inline static std::vector<std::string> names;

  SDCase() = default;
  constexpr explicit SDCase(uint8_t v) noexcept : v(v) {}
  constexpr SDCase(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  explicit SDCase(std::string_view name) {
    if (names.empty()) {
      assert(name == "none" && "First SDCase constructed must be \"none\"");
    } else {
      assert(name != "none" && "\"none\" SDCase already exists");
    }
    names.push_back(std::string{name});
    v = static_cast<uint8_t>(names.size() - 1);
  }

  std::string show() const noexcept {
    if (static_cast<size_t>(v) >= names.size()) return "";
    return names[v];
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const SDCase &) const = default;
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
   Fixed wrappers use their nested Enum; ordinary enums use reflection directly.
   Runtime wrappers retain their names vector and uint8_t constructor.
   Unknown input returns std::nullopt; named zero sentinels parse successfully.
   Fixed wrappers use their nested Enum; ordinary enums use reflection directly.
   Runtime wrappers retain their names vector and uint8_t constructor.
   Unknown input returns std::nullopt; named zero sentinels parse successfully.
   Usage:
     auto ret = trait_from_string<Agegrp>("age20_39");
     if (!ret) { // handle bad input }
     Agegrp ag = *ret;
*/
template<typename T>
std::optional<T> trait_from_string(const std::string& s) {
  if constexpr (std::is_enum_v<T>) {
    return magic_enum::enum_cast<T>(s, magic_enum::case_insensitive);
  } else if constexpr (requires { typename T::Enum; }) {
    const auto value = magic_enum::enum_cast<typename T::Enum>(
        s, magic_enum::case_insensitive);
    if (!value) return std::nullopt;
    return T{*value};
  } else {
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
}
