#pragma once  
#include "lib_includes.h"  

// define trait classes and instance objects that do not change at runtime
/*
use as:

Status person_status = Stat::Unexposed;
person_status = Stat::Infectious;    // fine
person_status = Stat::Recovered;   // fine

*/


// Agegrp
struct Agegrp {
    uint8_t v{};
    
    constexpr explicit Agegrp(uint8_t v) noexcept : v(v) {}
    constexpr operator uint8_t() const noexcept { return v; }
    constexpr bool operator==(const Agegrp &) const = default;
    constexpr bool operator!=(const Agegrp &) const = default;
};

namespace Age {
  inline const Agegrp Unknown{0};
  inline const Agegrp Age0_19{1};
  inline const Agegrp Age20_39{2};
  inline const Agegrp Age40_59{3};
  inline const Agegrp Age60_79{4};
  inline const Agegrp Age80_Up{5};
}

// Status
struct Status {
    uint8_t v{};
    
    constexpr explicit Status(uint8_t v) noexcept : v(v) {}
    constexpr operator uint8_t() const noexcept { return v; }
    constexpr bool operator==(const Status &) const = default;
    constexpr bool operator!=(const Status &) const = default;
};

namespace Stat {
  inline const Status None{0};
  inline const Status Unexposed{1};
  inline const Status Infectious{2};
  inline const Status Recovered{3};
  inline const Status Dead{4};}

// Condition
struct Condition {
    uint8_t v{};
    
    constexpr explicit Condition(uint8_t v) noexcept : v(v) {}
    constexpr operator uint8_t() const noexcept { return v; }
    constexpr bool operator==(const Condition &) const = default;
    constexpr bool operator!=(const Condition &) const = default;
};

namespace Cond {
  inline const Condition Uninfected{0};
  inline const Condition Nil{1};
  inline const Condition Mild{2};
  inline const Condition Sick{3};
  inline const Condition Severe{4};
}

// Progressionmap
struct Progressionmap {
    uint8_t v{};
    
    constexpr explicit Progressionmap(uint8_t v) noexcept : v(v) {}
    constexpr operator uint8_t() const noexcept { return v; }
    constexpr bool operator==(const Progressionmap &) const = default;
    constexpr bool operator!=(const Progressionmap &) const = default;
};

namespace Progressmap {
  inline const Progressionmap ToRecover{0};
  inline const Progressionmap ToNil{1};
  inline const Progressionmap ToMild{2};
  inline const Progressionmap ToSick{3};
  inline const Progressionmap ToSevere{4};
  inline const Progressionmap ToDead{5};
}
