#include "traits.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <magic_enum.hpp>
#include <fmt/base.h>
#include <fmt/format.h> 


// TraitID lookups
// note: trait value lookups follow trait_table definition
std::string trait_lookup(TraitID id) {
  return std::string(magic_enum::enum_name(id));
}
std::string trait_lookup(uint8_t id) {
  return trait_lookup(static_cast<TraitID>(id));
}
uint8_t trait_lookup(std::string str) {
  auto res = magic_enum::enum_cast<TraitID>(str, magic_enum::case_insensitive);
  if (res) return static_cast<uint8_t>(*res);
  else return 0;    // this is also the value for trait "none":  not all callees may process correctly
}


/*
array trait_table is map to names and count lookup for all Trait classes in PopData columns
*/
inline constexpr std::array trait_table{
  TraitEntry{.id = TraitID::none},
  TraitEntry{.id = TraitID::status, .names = Status::names, .count = Status::count},
  TraitEntry{.id = TraitID::vax, .names = Vax::names, .count = Vax::count},
  TraitEntry{.id = TraitID::variant, .names = Variant::names, .count = Variant::count},
  TraitEntry{.id = TraitID::ring, .names = Ring::names, .count = Ring::count},
  TraitEntry{.id = TraitID::agegrp, .names = Agegrp::names, .count = Agegrp::count},
  TraitEntry{.id = TraitID::condition, .names = Condition::names, .count = Condition::count},
  TraitEntry{.id = TraitID::vaxstatus, .names = Vaxstatus::names, .count = Vaxstatus::count},
  TraitEntry{.id = TraitID::duration, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::sickday, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::recovday, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::deadday, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::testday, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::quarday, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::vaxday, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::sdcase, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::varianthist, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::vaxhist, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::sickdayhist, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::recovdayhist, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::testdayhist, .names = no_names, .count = no_count},
  TraitEntry{.id = TraitID::vaxdayhist, .names = no_names, .count = no_count},
};

std::optional<uint8_t> trait_name_to_id(
    const std::vector<std::string>& names, const std::string& name,
    bool case_insensitive) {
  for (size_t i = 0; i < names.size(); ++i) {
    const auto& candidate = names[i];
    const bool matches = case_insensitive
        ? std::equal(candidate.begin(), candidate.end(), name.begin(), name.end(),
            [](unsigned char a, unsigned char b) {
              return std::tolower(a) == std::tolower(b);
            })
        : candidate == name;
    if (matches) {
      if (i > std::numeric_limits<uint8_t>::max()) {
        throw std::out_of_range("Trait name index exceeds uint8_t storage");
      }
      return static_cast<uint8_t>(i);
    }
  }
  return std::nullopt;
}

void validate_fixed_trait_names() {
  for (const auto id : {TraitID::agegrp, TraitID::status,
                        TraitID::condition, TraitID::vaxstatus}) {
    const auto& names = trait_table[std::to_underlying(id)].names;
    if (names.empty() || names.size() > size_t{std::numeric_limits<uint8_t>::max()} + 1) {
      throw std::logic_error(fmt::format("Invalid names table size for {}", trait_lookup(id)));
    }
    for (size_t i = 0; i < names.size(); ++i) {
      if (names[i].empty() ||
          std::find(names.begin(), names.begin() + i, names[i]) != names.begin() + i) {
        throw std::logic_error(fmt::format("Empty or duplicate name for {}", trait_lookup(id)));
      }
    }
  }
  if (Agegrp::names.size() != Agegrp::count + 1 ||
      Condition::names.size() != Condition::count + 1) {
    throw std::logic_error("Fixed trait names do not match progression array dimensions");
  }
}

// trait value lookups
std::string_view tr_val_to_string(TraitID id, uint8_t v) {
  auto tbl_idx = std::to_underlying(id);
  if (tbl_idx < 1 || tbl_idx > trait_table.size() - 1) 
    throw std::out_of_range("trait id out of range to string");
  const auto& e = trait_table[std::to_underlying(id)];
  if (e.count == 0 || v >= e.names.size())
    throw std::out_of_range("index to trait value out of range");
  return e.names[v];
}

uint8_t tr_val_to_id(TraitID id, std::string_view name) {
  auto tbl_idx = std::to_underlying(id);
  return tr_val_to_id(tbl_idx, name);
}

uint8_t tr_val_to_id(uint8_t id, std::string_view name) {
  if (id < 1 || id > trait_table.size() - 1) 
    throw std::out_of_range("trait id out of range of the trait_table.");
  const auto& trait = trait_table[id];
  if (trait.count == 0) 
    throw std::out_of_range("trait name selected for a trait with no members.");
  if (const auto value = trait_name_to_id(trait.names, std::string{name})) return *value;
  throw std::invalid_argument(fmt::format("unknown name {} for trait value {}", name, static_cast<uint8_t>(id)));
}
