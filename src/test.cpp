#include <string>
#include <iostream>

using std::string;

#include "epi_sim.h"
#include "categories.h"

void run_category_tests() {
    std::cout << "=== Testing New Namespace Approach ===\n\n";

    // Test Status namespace
    std::cout << "--- Testing Status Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Status::none = " << static_cast<int>(Status::none) << " (expected 0)\n";
    std::cout << "  Status::unexposed = " << static_cast<int>(Status::unexposed) << " (expected 1)\n";
    std::cout << "  Status::infectious = " << static_cast<int>(Status::infectious) << " (expected 2)\n";
    std::cout << "  Status::recovered = " << static_cast<int>(Status::recovered) << " (expected 3)\n";
    std::cout << "  Status::dead = " << static_cast<int>(Status::dead) << " (expected 4)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Status::to_str(0) = \"" << Status::to_str(0) << "\" (expected \"none\")\n";
    std::cout << "  Status::to_str(1) = \"" << Status::to_str(1) << "\" (expected \"unexposed\")\n";
    std::cout << "  Status::to_str(2) = \"" << Status::to_str(2) << "\" (expected \"infectious\")\n";
    std::cout << "  Status::to_str(3) = \"" << Status::to_str(3) << "\" (expected \"recovered\")\n";
    std::cout << "  Status::to_str(4) = \"" << Status::to_str(4) << "\" (expected \"dead\")\n";
    std::cout << "  Status::from_str(\"unexposed\") = " << static_cast<int>(Status::from_str("unexposed")) << " (expected 1)\n";
    std::cout << "  Status::from_str(\"infectious\") = " << static_cast<int>(Status::from_str("infectious")) << " (expected 2)\n";
    std::cout << "  Status::from_str(\"recovered\") = " << static_cast<int>(Status::from_str("recovered")) << " (expected 3)\n";
    std::cout << "  Status::from_str(\"dead\") = " << static_cast<int>(Status::from_str("dead")) << " (expected 4)\n";
    std::cout << "  Status::from_str(\"invalid\") = " << static_cast<int>(Status::from_str("invalid")) << " (expected 0 - default to none)\n";
    std::cout << "\n";

    // Test Condition namespace
    std::cout << "--- Testing Condition Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Condition::uninfected = " << static_cast<int>(Condition::uninfected) << " (expected 0)\n";
    std::cout << "  Condition::nil = " << static_cast<int>(Condition::nil) << " (expected 1)\n";
    std::cout << "  Condition::mild = " << static_cast<int>(Condition::mild) << " (expected 2)\n";
    std::cout << "  Condition::sick = " << static_cast<int>(Condition::sick) << " (expected 3)\n";
    std::cout << "  Condition::severe = " << static_cast<int>(Condition::severe) << " (expected 4)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Condition::to_str(0) = \"" << Condition::to_str(0) << "\" (expected \"uninfected\")\n";
    std::cout << "  Condition::to_str(1) = \"" << Condition::to_str(1) << "\" (expected \"nil\")\n";
    std::cout << "  Condition::to_str(2) = \"" << Condition::to_str(2) << "\" (expected \"mild\")\n";
    std::cout << "  Condition::to_str(3) = \"" << Condition::to_str(3) << "\" (expected \"sick\")\n";
    std::cout << "  Condition::to_str(4) = \"" << Condition::to_str(4) << "\" (expected \"severe\")\n";
    std::cout << "  Condition::from_str(\"uninfected\") = " << static_cast<int>(Condition::from_str("uninfected")) << " (expected 0)\n";
    std::cout << "  Condition::from_str(\"nil\") = " << static_cast<int>(Condition::from_str("nil")) << " (expected 1)\n";
    std::cout << "  Condition::from_str(\"mild\") = " << static_cast<int>(Condition::from_str("mild")) << " (expected 2)\n";
    std::cout << "  Condition::from_str(\"sick\") = " << static_cast<int>(Condition::from_str("sick")) << " (expected 3)\n";
    std::cout << "  Condition::from_str(\"severe\") = " << static_cast<int>(Condition::from_str("severe")) << " (expected 4)\n";
    std::cout << "  Condition::from_str(\"invalid\") = " << static_cast<int>(Condition::from_str("invalid")) << " (expected 0 - default to uninfected)\n";
    std::cout << "\n";

    // Test Agegrp namespace
    std::cout << "--- Testing Agegrp Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Agegrp::unknown = " << static_cast<int>(Agegrp::unknown) << " (expected 0)\n";
    std::cout << "  Agegrp::age0_19 = " << static_cast<int>(Agegrp::age0_19) << " (expected 1)\n";
    std::cout << "  Agegrp::age20_39 = " << static_cast<int>(Agegrp::age20_39) << " (expected 2)\n";
    std::cout << "  Agegrp::age40_59 = " << static_cast<int>(Agegrp::age40_59) << " (expected 3)\n";
    std::cout << "  Agegrp::age60_79 = " << static_cast<int>(Agegrp::age60_79) << " (expected 4)\n";
    std::cout << "  Agegrp::age80_up = " << static_cast<int>(Agegrp::age80_up) << " (expected 5)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Agegrp::to_str(0) = \"" << Agegrp::to_str(0) << "\" (expected \"unknown\")\n";
    std::cout << "  Agegrp::to_str(1) = \"" << Agegrp::to_str(1) << "\" (expected \"age0_19\")\n";
    std::cout << "  Agegrp::to_str(2) = \"" << Agegrp::to_str(2) << "\" (expected \"age20_39\")\n";
    std::cout << "  Agegrp::to_str(3) = \"" << Agegrp::to_str(3) << "\" (expected \"age40_59\")\n";
    std::cout << "  Agegrp::to_str(4) = \"" << Agegrp::to_str(4) << "\" (expected \"age60_79\")\n";
    std::cout << "  Agegrp::to_str(5) = \"" << Agegrp::to_str(5) << "\" (expected \"age80_up\")\n";
    std::cout << "  Agegrp::from_str(\"age0_19\") = " << static_cast<int>(Agegrp::from_str("age0_19")) << " (expected 1)\n";
    std::cout << "  Agegrp::from_str(\"age20_39\") = " << static_cast<int>(Agegrp::from_str("age20_39")) << " (expected 2)\n";
    std::cout << "  Agegrp::from_str(\"age40_59\") = " << static_cast<int>(Agegrp::from_str("age40_59")) << " (expected 3)\n";
    std::cout << "  Agegrp::from_str(\"age60_79\") = " << static_cast<int>(Agegrp::from_str("age60_79")) << " (expected 4)\n";
    std::cout << "  Agegrp::from_str(\"age80_up\") = " << static_cast<int>(Agegrp::from_str("age80_up")) << " (expected 5)\n";
    std::cout << "  Agegrp::from_str(\"invalid\") = " << static_cast<int>(Agegrp::from_str("invalid")) << " (expected 0 - default to unknown)\n";
    std::cout << "\n";

    // Test Vaccine namespace
    std::cout << "--- Testing Vaccine Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Vaccine::none = " << static_cast<int>(Vaccine::none) << " (expected 0)\n";
    std::cout << "  Vaccine::Pfizer = " << static_cast<int>(Vaccine::Pfizer) << " (expected 1)\n";
    std::cout << "  Vaccine::Moderna = " << static_cast<int>(Vaccine::Moderna) << " (expected 2)\n";
    std::cout << "  Vaccine::JnJ = " << static_cast<int>(Vaccine::JnJ) << " (expected 3)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Vaccine::to_str(0) = \"" << Vaccine::to_str(0) << "\" (expected \"none\")\n";
    std::cout << "  Vaccine::to_str(1) = \"" << Vaccine::to_str(1) << "\" (expected \"Pfizer\")\n";
    std::cout << "  Vaccine::to_str(2) = \"" << Vaccine::to_str(2) << "\" (expected \"Moderna\")\n";
    std::cout << "  Vaccine::to_str(3) = \"" << Vaccine::to_str(3) << "\" (expected \"J & J\")\n";
    std::cout << "  Vaccine::from_str(\"none\") = " << static_cast<int>(Vaccine::from_str("none")) << " (expected 0)\n";
    std::cout << "  Vaccine::from_str(\"Pfizer\") = " << static_cast<int>(Vaccine::from_str("Pfizer")) << " (expected 1)\n";
    std::cout << "  Vaccine::from_str(\"Moderna\") = " << static_cast<int>(Vaccine::from_str("Moderna")) << " (expected 2)\n";
    std::cout << "  Vaccine::from_str(\"J & J\") = " << static_cast<int>(Vaccine::from_str("J & J")) << " (expected 3)\n";
    std::cout << "  Vaccine::from_str(\"JnJ\") = " << static_cast<int>(Vaccine::from_str("JnJ")) << " (expected 3 - alternate spelling)\n";
    std::cout << "  Vaccine::from_str(\"invalid\") = " << static_cast<int>(Vaccine::from_str("invalid")) << " (expected 0 - default to none)\n";
    std::cout << "\n";

    // Test boundary conditions
    std::cout << "--- Testing Boundary Conditions ---\n";
    std::cout << "  Status::to_str(99) = \"" << Status::to_str(99) << "\" (expected \"unknown\" - out of range)\n";
    std::cout << "  Condition::to_str(99) = \"" << Condition::to_str(99) << "\" (expected \"unknown\" - out of range)\n";
    std::cout << "  Agegrp::to_str(99) = \"" << Agegrp::to_str(99) << "\" (expected \"unknown\" - out of range)\n";
    std::cout << "  Vaccine::to_str(99) = \"" << Vaccine::to_str(99) << "\" (expected \"unknown\" - out of range)\n";
    std::cout << "\n";

    std::cout << "=== All namespace tests completed ===\n";
}

int main() {
  run_category_tests();
  return 0;
}