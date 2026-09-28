#include <cstdlib>
#include <magic_enum/magic_enum.hpp>
#include <fmt/base.h>
#include <fmt/format.h> // only get what I use: about 12k in the executable!
#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace me = magic_enum;


//
// run the application with immediate commands that show information and stop or
// run the simulation with an input case
//

enum class Trait : uint8_t {
    status = 0,
    vax = 1,
    variant = 2
};

template <typename T>
constexpr std::string_view type_label() {
    using U = std::remove_cvref_t<T>;

    if constexpr (std::same_as<U, std::size_t>) {
        return "size_t";
    } else if constexpr (std::same_as<U, std::uint8_t>) {
        return "uint8_t";
    } else if constexpr (std::same_as<U, int>) {
        return "int";
    } else if constexpr (std::same_as<U, double>) {
        return "double";
    } else {
        return "unknown";
    }
}

int main(int argc, char** argv) {
  // example commands with magic_enum

  // enum class (type name) of an enum member:  enum_type_bame<decltype()>
  Trait st = Trait::status;
  fmt::println("the value comes from enum class: {}", me::enum_type_name<decltype(st)>());
  
  // enum member to string: enum_name()
  fmt::println("The name for this enum value is: {}", me::enum_name(st));

  // enum member to integer: enum_integer()
  fmt::println("The integer value of this enum is: {}", me::enum_integer(st));

  // string to enum member: function enum_cast<`the enum class`>(`a string`)
  // like this:   me::enum_cast<Trait>("status").value()
  auto str_selector = "status";
  fmt::println("The enum for the string \"status\" should convert back to \"status\": {}", me::enum_name(me::enum_cast<Trait>(str_selector).value()));

  // integer to enum member:  enum_cast<`the enum class`>(`an int`)
  // like this:   me::enum_cast<Trait>("status").value()
  auto int_selector = 0;
  fmt::println("The enum for the integer 0 should access the Trait member and convert back to \"status\": {}", me::enum_name(me::enum_cast<Trait>(int_selector).value()));

  // underlying type from enum class name
  size_t idx{1};
  fmt::println("Type of variable idx is: {}", type_label<decltype(idx)>());
  fmt::println("Underlying type of Trait is: {}",type_label<me::underlying_type_t<Trait>>());
  return 0;
}
