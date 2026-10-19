#include "traits.h"

//
// array trait_table is map to names and count lookup for all Trait classes
//
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
