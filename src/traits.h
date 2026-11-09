#pragma once

#include "traits.h"
#include "helpers.h"
#include <array>
#include <cstdint>
#include <concepts>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <magic_enum/magic_enum.hpp>
#include <array>
#include <cstddef>
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

//
// registry of all traits by id and name
// these are all used in PopData associated with 1 column
// some are used for history vectors in Histories
//
enum class TraitID : uint8_t {
  none = 0, 
  status,
  vax,
  variant,
  ring,
  agegrp, 
  condition,
  vaxstatus,
  duration,
  sickday,
  recovday,
  deadday,
  testday,
  quarday,
  vaxday,
  sdcase,
  varianthist,
  vaxhist,
  sickdayhist,
  recovdayhist,
  testdayhist,
  vaxdayhist
};



// sentinels for traits that don't have count or names members
inline const std::vector<std::string> no_names{};
inline const uint16_t no_count = 0;
// 
// TraitEntry holds central defintions for traits that *may* be
// used as history vectors
//
struct TraitEntry {
  TraitID id;
  const std::vector<std::string> & names = no_names;
  const uint16_t & count = no_count;
};

/*
array trait_table is map to names and count lookup for all Trait classes in PopData columns to centralize access to names, count, and reverse lookups
The definition is near the top of traits.cpp.
*/
extern const std::array<
    TraitEntry, magic_enum::enum_count<TraitID>()> trait_table;



// lookups via the TraitID enum
std::string trait_lookup(TraitID id);
std::string trait_lookup(uint8_t id);
uint8_t trait_lookup(std::string str);

// lookups for trait values
std::string_view tr_val_to_string(TraitID id, uint8_t v);
uint8_t tr_val_to_id(TraitID id, std::string_view name);
uint8_t tr_val_to_id(uint8_t id, std::string_view name);

std::optional<uint8_t> trait_name_to_id(
    const std::vector<std::string>& names, const std::string& name,
    bool case_insensitive = false);
void validate_fixed_trait_names();


/*
Traits whose name tables are defined in source and immutable after initialization

use as:

  Status person_status = UNEXPOSED;
  person_status = INFECTIOUS;    
  person_status = RECOVERED;   
*/

//
// Agegrp: fixed names with one-byte PopData storage.
//
struct Agegrp {
  uint8_t v{};
  inline static const std::vector<std::string> names{
      "unknown", "age0_19", "age20_39", "age40_59", "age60_79", "age80_up"};
  // A compile-time dimension of the progression arrays; checked at setup.
  static constexpr uint16_t count = 5;

  template <std::integral I>
  explicit Agegrp(I value) {
    if (value < 0 || static_cast<uintmax_t>(value) >= names.size()) {
      throw std::out_of_range("Invalid Agegrp value");
    }
    v = static_cast<uint8_t>(value);
  }

  Agegrp() = default;
  Agegrp(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
      const auto value = trait_name_to_id(names, name, true);
      if (!value) {
        throw std::invalid_argument(fmt::format("Unknown Agegrp name '{}'", name));
      }
      return *value;
  }

  std::string show() const {
    return v < names.size() ? names[v] : std::string{};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Agegrp&) const = default;
};



static_assert(sizeof(Agegrp) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Agegrp>);

// constants for Agegrp instances used in PopData and Series
inline const Agegrp UNKNOWN{"unknown"};
inline const Agegrp AGE0_19{"age0_19"};
inline const Agegrp AGE20_39{"age20_39"};
inline const Agegrp AGE40_59{"age40_59"};
inline const Agegrp AGE60_79{"age60_79"};
inline const Agegrp AGE80_UP{"age80_up"};

//
// Status: fixed names with one-byte PopData storage.
//
struct Status {
  uint8_t v{};
  inline static const std::vector<std::string> names{
      "none", "unexposed", "infectious", "recovered", "dead"};
  inline static const uint16_t count = static_cast<uint16_t>(names.size() - 1);

  Status() = default;
  template <std::integral I>
  explicit Status(I value) {
    if (value < 0 || static_cast<uintmax_t>(value) >= names.size()) {
      throw std::out_of_range("Invalid Status value");
    }
    v = static_cast<uint8_t>(value);
  }
  Status(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    return trait_name_to_id(names, name, true).value_or(0);
  }

  std::string show() const {
    return v < names.size() ? names[v] : std::string{};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Status&) const = default;
};

static_assert(sizeof(Status) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Status>);

// constants for Status instances used in PopData and Series
inline const Status NONE{"none"};
inline const Status UNEXPOSED{"unexposed"};
inline const Status INFECTIOUS{"infectious"};
inline const Status RECOVERED{"recovered"};
inline const Status DEAD{"dead"};

//
// Condition: fixed names with one-byte PopData storage.
//
struct Condition {
  uint8_t v{};
  inline static const std::vector<std::string> names{
      "error", "uninfected", "nil", "mild", "sick", "severe"};
  // A compile-time dimension of the progression arrays; checked at setup.
  static constexpr uint16_t count = 5;

  Condition() = default;
  template <std::integral I>
  explicit Condition(I value) {
    if (value < 0 || static_cast<uintmax_t>(value) >= names.size()) {
      throw std::out_of_range("Invalid Condition value");
    }
    v = static_cast<uint8_t>(value);
  }
  Condition(std::string name) : v(resolve_name(std::move(name))) {}

  static uint8_t resolve_name(std::string name) {
    return trait_name_to_id(names, name, true).value_or(1);
  }

  std::string show() const {
    return v < names.size() ? names[v] : std::string{};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Condition&) const = default;
};

static_assert(sizeof(Condition) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Condition>);

// constants for Condition instances used in PopData
inline const Condition UNINFECTED{"uninfected"};
inline const Condition NIL{"nil"};
inline const Condition MILD{"mild"};
inline const Condition SICK{"sick"};
inline const Condition SEVERE{"severe"};

/* 
Vaxstatus: fixed names with one-byte PopData storage.
values: none=0, first, full, booster, exhausted
*/
struct Vaxstatus {
  uint8_t v{};
  inline static const std::vector<std::string> names{
      "none", "first", "full", "booster", "exhausted"};
  inline static const uint16_t count = static_cast<uint16_t>(names.size());

  template <std::integral I>
  explicit Vaxstatus(I value) {
    if (value < 0 || static_cast<uintmax_t>(value) >= names.size()) {
      throw std::out_of_range("Invalid Vaxstatus value");
    }
    v = static_cast<uint8_t>(value);
  }

  std::string show() const noexcept {
    return v < names.size() ? names[v] : std::string{};
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Vaxstatus&) const = default;
};

static_assert(sizeof(Vaxstatus) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Vaxstatus>);

static_assert(sizeof(Vaxstatus) == sizeof(uint8_t));
static_assert(std::is_trivially_copyable_v<Vaxstatus>);

namespace Vaxstat {
  inline const Vaxstatus none{trait_name_to_id(Vaxstatus::names, "none").value()};
  inline const Vaxstatus first{trait_name_to_id(Vaxstatus::names, "first").value()};
  inline const Vaxstatus full{trait_name_to_id(Vaxstatus::names, "full").value()};
  inline const Vaxstatus booster{trait_name_to_id(Vaxstatus::names, "booster").value()};
  inline const Vaxstatus exhausted{trait_name_to_id(Vaxstatus::names, "exhausted").value()};
} // namespace Vaxstat

//
// All six progression outcomes are valid zero-based parameter indices.
// Note:  not used in PopData; not associated with any Histories vector
// Not compliant with the Trait concept
enum class Progressionmap : uint8_t {
  ToRecover = 0, ToNil, ToMild, ToSick, ToSevere, ToDead
};

namespace Progressmap {
  inline constexpr auto ToRecover = Progressionmap::ToRecover;
  inline constexpr auto ToNil = Progressionmap::ToNil;
  inline constexpr auto ToMild = Progressionmap::ToMild;
  inline constexpr auto ToSick = Progressionmap::ToSick;
  inline constexpr auto ToSevere = Progressionmap::ToSevere;
  inline constexpr auto ToDead = Progressionmap::ToDead;
}

// do_progression maps outcomes 1..4 to Condition IDs 2..5 (nil..severe).

//
// length of time being infected, per each infection incidence
// has no string representation
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

//
// Ring
//
struct Ring {
  uint8_t v{};
  inline static std::vector<std::string> names;
  inline static uint16_t count{0};

  constexpr Ring() noexcept = default;
  // explicit constexpr Ring(uint8_t value) noexcept : v(value) {}
  explicit Ring(std::string_view name) {
    if (names.empty()) {
      names.emplace_back("");  // index 0 reserved as sentinel for population
    }
    assert(!name.empty() && "Ring name must not be empty");
    if (names.size() < 255) {
      names.emplace_back(std::string{name});
      v = static_cast<uint8_t>(names.size() - 1);
      count++;}
    else throw std::invalid_argument("Ring trait value must fit in uint8_t raw values @ <= 255");
  }
  Ring(int val) {
    if (val > 0 && val <= names.size())  v = static_cast<uint8_t>(val);
    else throw std::invalid_argument("Ring trait value exceeds number of rings.");
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

//
// Sickday
//
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

// 
// Recovday
//
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

// 
// Deadday
//
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

//
// Testday
//
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

//
// Quarday
//
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

//
// Vaxday
//
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

//
// Variant -- create instances at runtime
// this trait has history vectors and history total vectors
//
struct Variant {
  uint8_t v{};  // will take on values 0..number of variants:  effectively 1-indexed
  inline static std::vector<std::string> names;
  inline static uint16_t count{0};

  Variant() = default;
  // constexpr explicit Variant(uint8_t v) noexcept : v(v) {}
  // constexpr Variant(int val) noexcept : v(static_cast<uint8_t>(val)) {}
  constexpr Variant(int val) {
    if (val <= 255 && val >= 0) v = static_cast<uint8_t>(val);
    else throw std::invalid_argument("Variant trait value must fit in uint8_t raw values @ <= 255");
  };
  explicit Variant(std::string_view name) {
    if (names.empty()) {
      assert(name == "none" && "First Variant constructed must be \"none\"");
    } else {
      assert(name != "none" && "\"none\" variant already exists");
    }
    if (names.size() < 255) {
      names.push_back(std::string{name});
      v = static_cast<uint8_t>(names.size() - 1);
      count++;}
    else throw std::invalid_argument("Variant trait value must fit in uint8_t raw values @ <= 255");
  }

  std::string show() const noexcept {
    if (static_cast<size_t>(v) >= names.size()) return "";
    return names[v];
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Variant &) const = default;
};

//
// this trait has history vectors and history total vectors
//
struct Vax {
  uint8_t v{};
  inline static std::vector<std::string> names;
  inline static uint16_t count{0};

  Vax() = default;
  // constexpr explicit Vax(uint8_t v) noexcept : v(v) {}
  constexpr Vax(int val) {
    if (val <= 255 && val >= 0) v = static_cast<uint8_t>(val);
    else throw std::invalid_argument("Vax trait value must fit in uint8_t raw values @ <= 255");
  };
  explicit Vax(std::string_view name) {
    if (names.empty()) {
      assert(name == "none" && "First Vax constructed must be \"none\"");
    } else {
      assert(name != "none" && "\"none\" vax already exists");
    }
    if (names.size() < 255) {
      names.push_back(std::string{name});
      v = static_cast<uint8_t>(names.size() - 1);
      count++;}
    else throw std::invalid_argument("Vax trait value must fit in uint8_t raw values @ <= 255");
  }

  std::string show() const noexcept {
    if (static_cast<size_t>(v) >= names.size()) return "";
    return names[v];
  }
  constexpr operator uint8_t() const noexcept { return v; }
  constexpr bool operator==(const Vax &) const = default;
};

//
// SDCase
//
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

//
// VariantHist
//
struct VariantHist {
  std::array<Variant, 16> arr{};
  uint16_t count{};

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

//
// VaxHist
//
struct VaxHist {
  std::array<Vax, 16> arr{};
  uint16_t count{};

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

//
// SickdayHist
//
struct SickdayHist {
  std::array<int16_t, 16> arr{};
  uint16_t count{};

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

//
// RecovdayHist
//
struct RecovdayHist {
  std::array<int16_t, 16> arr{};
  uint16_t count{};

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

//
// TestdayHist
//
struct TestdayHist {
  std::array<int16_t, 16> arr{};
  uint16_t count{};

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

//
// VaxdayHist
//
struct VaxdayHist {
  std::array<int16_t, 16> arr{};
  uint16_t count{};

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
   Wrappers use their names vectors; ordinary enums use reflection directly.
   Fixed wrappers use checked construction; runtime wrappers retain sentinel handling.
   Unknown input returns std::nullopt; named zero sentinels parse successfully.

   Usage:
     auto ret = trait_from_string<Agegrp>("age20_39");
     if (!ret) { // handle bad input }
     Agegrp ag = *ret;
*/
// only used in tests and cases.cpp--so, not for series
template<typename T>
std::optional<T> trait_from_string(const std::string& s) {
  if constexpr (std::is_enum_v<T>) {
    return magic_enum::enum_cast<T>(s, magic_enum::case_insensitive);
  } else {
    const auto value = trait_name_to_id(T::names, s, true);
    if (!value) return std::nullopt;
    if constexpr (std::same_as<T, Agegrp> || std::same_as<T, Status> ||
                  std::same_as<T, Condition> || std::same_as<T, Vaxstatus>) {
      return T{*value};
    } else {
      T result{};
      result.v = *value;
      return result;
    }
  }
}
