#include <string>
#include <iostream>
#include <vector>

using std::string;
using std::vector;

#include "epi_sim.h"
#include "categories.h"
#include "population.h"

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


void test_popdata_constructor() {
    std::cout << "\n=== Testing PopData Constructor ===\n\n";

    std::cout << "Creating PopData with 10 people...\n";
    PopData pop(10);
    std::cout << "Constructor executed successfully!\n\n";

    std::cout << "Manually checking values for persons 0, 5, and 9:\n";
    std::cout << "Expected: status=1, agegrp=1, cond=1, duration=0, ring=0\n\n";

    std::cout << "Person 0:\n";
    std::cout << "  status = " << static_cast<int>(pop.status[0]) << " (expected 1)\n";
    std::cout << "  agegrp = " << static_cast<int>(pop.agegrp[0]) << " (expected 1)\n";
    std::cout << "  cond = " << static_cast<int>(pop.cond[0]) << " (expected 1)\n";
    std::cout << "  duration = " << static_cast<int>(pop.duration[0]) << " (expected 0)\n";
    std::cout << "  ring = " << static_cast<int>(pop.ring[0]) << " (expected 0)\n";
    std::cout << "  variant_count = " << static_cast<int>(pop.variant_count[0]) << " (expected 0)\n";
    std::cout << "  variant[0][0] = " << static_cast<int>(pop.variant[0][0]) << " (expected 0 - first element of array)\n\n";

    std::cout << "Person 5:\n";
    std::cout << "  status = " << static_cast<int>(pop.status[5]) << " (expected 1)\n";
    std::cout << "  agegrp = " << static_cast<int>(pop.agegrp[5]) << " (expected 1)\n";
    std::cout << "  cond = " << static_cast<int>(pop.cond[5]) << " (expected 1)\n";
    std::cout << "  vaxstatus = " << static_cast<int>(pop.vaxstatus[5]) << " (expected 0)\n";
    std::cout << "  vax_count = " << static_cast<int>(pop.vax_count[5]) << " (expected 0)\n\n";

    std::cout << "Person 9:\n";
    std::cout << "  status = " << static_cast<int>(pop.status[9]) << " (expected 1)\n";
    std::cout << "  agegrp = " << static_cast<int>(pop.agegrp[9]) << " (expected 1)\n";
    std::cout << "  deadday = " << static_cast<int>(pop.deadday[9]) << " (expected 0)\n";
    std::cout << "  quar = " << static_cast<int>(pop.quar[9]) << " (expected 0)\n";
    std::cout << "  quarday = " << static_cast<int>(pop.quarday[9]) << " (expected 0)\n\n";

    std::cout << "=== PopData Constructor Test Passed ===\n";
}


void test_popdata_print_table() {
    std::cout << "\n=== Testing PopData Print Table ===\n\n";

    std::cout << "Creating PopData with 100 people...\n";
    PopData pop(100);
    std::cout << "Constructor executed successfully!\n\n";

    // Select 5 rows to print (indices 0, 25, 50, 75, 99)
    vector<int> rows = {0, 25, 50, 75, 99};

    // Test 1: First 5 columns (simple uint8_t vectors)
    std::cout << "--- Test 1: Simple columns (status, agegrp, cond, duration, ring) ---\n";
    vector<PopData::Column> cols1 = {
        PopData::Column::status,
        PopData::Column::agegrp,
        PopData::Column::cond,
        PopData::Column::duration,
        PopData::Column::ring
    };
    std::cout << "Row:\tstatus | agegrp | cond | duration | ring |\n";
    std::cout << "------------------------------------------------------------\n";
    pop.print_table(rows, cols1);
    std::cout << "\n";

    // Test 2: Mix of simple and array columns
    std::cout << "--- Test 2: Mixed columns (variant, variant_count, sickday, sickday_count, deadday) ---\n";
    vector<PopData::Column> cols2 = {
        PopData::Column::variant,
        PopData::Column::variant_count,
        PopData::Column::sickday,
        PopData::Column::sickday_count,
        PopData::Column::deadday
    };
    std::cout << "Row:\tvariant | variant_count | sickday | sickday_count | deadday |\n";
    std::cout << "--------------------------------------------------------------------\n";
    pop.print_table(rows, cols2);
    std::cout << "\n";

    // Test 3: Vaccine-related columns
    std::cout << "--- Test 3: Vaccine columns (vaxstatus, vaxrcvd, vax_count, quar, quarday) ---\n";
    vector<PopData::Column> cols3 = {
        PopData::Column::vaxstatus,
        PopData::Column::vaxrcvd,
        PopData::Column::vax_count,
        PopData::Column::quar,
        PopData::Column::quarday
    };
    std::cout << "Row:\tvaxstatus | vaxrcvd | vax_count | quar | quarday |\n";
    std::cout << "------------------------------------------------------------\n";
    pop.print_table(rows, cols3);
    std::cout << "\n";

    std::cout << "=== PopData Print Table Test Completed ===\n";
}


int main() {
  // run_category_tests();
  test_popdata_constructor();
  test_popdata_print_table();
  return 0;
}