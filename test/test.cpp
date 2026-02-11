#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <iomanip>
#include <numeric>
#include "../src/parameters.h"
// #include <fmt/base.h>
// #include <fmt/format.h>
#include "../src/setup.cpp"
// #include "../src/epi_sim.h"
// #include "../src/categories.h"
// #include "../src/population.h"

using std::string;
using std::vector;
using std::cout;


void run_category_tests() {
    std::cout << "=== Testing New Namespace Approach ===\n\n";

    // Test Status namespace
    std::cout << "--- Testing Status Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Status::none = " << static_cast<int>(Traits::Status("none")) << " (expected 0)\n";
    std::cout << "  Traits::Status::unexposed = " << static_cast<int>(Traits::Status("unexposed")) << " (expected 1)\n";
    std::cout << "  Status::infectious = " << static_cast<int>(Traits::Status("infectious")) << " (expected 2)\n";
    std::cout << "  Status::recovered = " << static_cast<int>(Traits::Status("recovered")) << " (expected 3)\n";
    std::cout << "  Status::dead = " << static_cast<int>(Traits::Status("dead")) << " (expected 4)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Status::to_str(0) = \"" << Traits::Status.to_str(0) << "\" (expected \"none\")\n";
    std::cout << "  Traits::Status::to_str(1) = \"" << Traits::Status.to_str(1) << "\" (expected \"unexposed\")\n";
    std::cout << "  Status::to_str(2) = \"" << Traits::Status.to_str(2) << "\" (expected \"infectious\")\n";
    std::cout << "  Status::to_str(3) = \"" << Traits::Status.to_str(3) << "\" (expected \"recovered\")\n";
    std::cout << "  Status::to_str(4) = \"" << Traits::Status.to_str(4) << "\" (expected \"dead\")\n";
    std::cout << "  Status::from_str(\"unexposed\") = " << static_cast<int>(Traits::Status("unexposed")) << " (expected 1)\n";
    std::cout << "  Status::from_str(\"infectious\") = " << static_cast<int>(Traits::Status("infectious")) << " (expected 2)\n";
    std::cout << "  Status::from_str(\"recovered\") = " << static_cast<int>(Traits::Status("recovered")) << " (expected 3)\n";
    std::cout << "  Status::from_str(\"dead\") = " << static_cast<int>(Traits::Status("dead")) << " (expected 4)\n";
    std::cout << "  Status::from_str(\"invalid\") = "
              << static_cast<int>(Traits::Status("invalid"))
              << " (expected 99 - default to none)\n";
    std::cout << "\n";

    // Test Condition namespace
    std::cout << "--- Testing Condition Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Condition::uninfected = " << static_cast<int>(Traits::Condition("uninfected")) << " (expected 0)\n";
    std::cout << "  Condition::nil = " << static_cast<int>(Traits::Condition("nil")) << " (expected 1)\n";
    std::cout << "  Condition::mild = " << static_cast<int>(Traits::Condition("mild")) << " (expected 2)\n";
    std::cout << "  Condition::sick = " << static_cast<int>(Traits::Condition("sick")) << " (expected 3)\n";
    std::cout << "  Condition::severe = " << static_cast<int>(Traits::Condition("severe")) << " (expected 4)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Condition::to_str(0) = \"" << Traits::Condition.to_str(0) << "\" (expected \"uninfected\")\n";
    std::cout << "  Condition::to_str(1) = \"" << Traits::Condition.to_str(1) << "\" (expected \"nil\")\n";
    std::cout << "  Condition::to_str(2) = \"" << Traits::Condition.to_str(2) << "\" (expected \"mild\")\n";
    std::cout << "  Condition::to_str(3) = \"" << Traits::Condition.to_str(3) << "\" (expected \"sick\")\n";
    std::cout << "  Condition::to_str(4) = \"" << Traits::Condition.to_str(4) << "\" (expected \"severe\")\n";
    std::cout << "  Condition::from_str(\"uninfected\") = " << static_cast<int>(Traits::Condition("uninfected")) << " (expected 0)\n";
    std::cout << "  Condition::from_str(\"nil\") = " << static_cast<int>(Traits::Condition("nil")) << " (expected 1)\n";
    std::cout << "  Condition::from_str(\"mild\") = " << static_cast<int>(Traits::Condition("mild")) << " (expected 2)\n";
    std::cout << "  Condition::from_str(\"sick\") = " << static_cast<int>(Traits::Condition("sick")) << " (expected 3)\n";
    std::cout << "  Condition::from_str(\"severe\") = " << static_cast<int>(Traits::Condition("severe")) << " (expected 4)\n";
    std::cout << "  Condition::from_str(\"invalid\") = "
              << static_cast<int>(Traits::Condition("invalid"))
              << " (expected 99 - default to uninfected)\n";
    std::cout << Traits::Condition.to_str(0) << " expected \"uninfected\"\n";
  
    std::cout << "\n";

    // Test Agegrp namespace
    std::cout << "--- Testing Agegrp Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Agegrp::unknown = " << static_cast<int>(Traits::Agegrp("unknown")) << " (expected 0)\n";
    std::cout << "  Agegrp::age0_19 = " << static_cast<int>(Traits::Agegrp("age0_19")) << " (expected 1)\n";
    std::cout << "  Agegrp::age20_39 = " << static_cast<int>(Traits::Agegrp("age20_39")) << " (expected 2)\n";
    std::cout << "  Agegrp::age40_59 = " << static_cast<int>(Traits::Agegrp("age40_59")) << " (expected 3)\n";
    std::cout << "  Agegrp::age60_79 = " << static_cast<int>(Traits::Agegrp("age60_79")) << " (expected 4)\n";
    std::cout << "  Agegrp::age80_up = " << static_cast<int>(Traits::Agegrp("age80_up")) << " (expected 5)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Agegrp::to_str(0) = \"" << Traits::Agegrp.to_str(0) << "\" (expected \"unknown\")\n";
    std::cout << "  Agegrp::to_str(1) = \"" << Traits::Agegrp.to_str(1) << "\" (expected \"age0_19\")\n";
    std::cout << "  Agegrp::to_str(2) = \"" << Traits::Agegrp.to_str(2) << "\" (expected \"age20_39\")\n";
    std::cout << "  Agegrp::to_str(3) = \"" << Traits::Agegrp.to_str(3) << "\" (expected \"age40_59\")\n";
    std::cout << "  Agegrp::to_str(4) = \"" << Traits::Agegrp.to_str(4) << "\" (expected \"age60_79\")\n";
    std::cout << "  Agegrp::to_str(5) = \"" << Traits::Agegrp.to_str(5) << "\" (expected \"age80_up\")\n";
    std::cout << "  Agegrp::from_str(\"age0_19\") = " << static_cast<int>(Traits::Agegrp("age0_19")) << " (expected 1)\n";
    std::cout << "  Agegrp::from_str(\"age20_39\") = " << static_cast<int>(Traits::Agegrp("age20_39")) << " (expected 2)\n";
    std::cout << "  Agegrp::from_str(\"age40_59\") = " << static_cast<int>(Traits::Agegrp("age40_59")) << " (expected 3)\n";
    std::cout << "  Agegrp::from_str(\"age60_79\") = " << static_cast<int>(Traits::Agegrp("age60_79")) << " (expected 4)\n";
    std::cout << "  Agegrp::from_str(\"age80_up\") = " << static_cast<int>(Traits::Agegrp("age80_up")) << " (expected 5)\n";
    std::cout << "  Agegrp::from_str(\"invalid\") = " << static_cast<int>(Traits::Agegrp("invalid")) << " (expected 99 - default to unknown)\n";
    std::cout << "\n";

    // Test boundary conditions
    std::cout << "--- Testing Boundary Conditions ---\n";
    std::cout << "  Status::to_str(99) = \"" << Traits::Status.to_str(99) << "\" (expected \"INVALID\" - out of range)\n";
    std::cout << "  Condition::to_str(99) = \"" << Traits::Condition.to_str(99) << "\" (expected \"INVALID\" - out of range)\n";
    std::cout << "  Agegrp::to_str(99) = \"" << Traits::Agegrp.to_str(99) << "\" (expected \"INVALID\" - out of range)\n";
    // std::cout << "  Vaccine::to_str(99) = \"" << Vaccine::to_str(99) << "\" (expected \"unknown\" - out of range)\n";
    std::cout << "\n";

    std::cout << "=== All namespace tests completed ===\n";
}

// Create a short alias for PopData::Column to use throughout this file
using PC = PopData::Column;

void test_popdata_print_table(PopData pop) {
    std::cout << "\n=== Testing PopData Print Table ===\n\n";



    // Select 5 rows to print (indices 0, 25, 50, 75, 99)
    vector<int> rows = {0, 25, 50, 75, 99};

    // Test 1: First 5 columns (simple uint8_t vectors)
    std::cout << "--- Test 1: Simple columns (status, agegrp, cond, duration, ring) ---\n";
    vector<PopData::Column> cols1 = {
        PC::status, 
        PC::agegrp,
        PC::cond,
        PC::duration,
        PC::ring
    };
    std::cout << "Row:\tstatus | agegrp | cond | duration | ring |\n";
    std::cout << "------------------------------------------------------------\n";
    pop.print_table(rows, cols1);
    std::cout << "\n";

    // Test 2: Mix of simple and array columns
    std::cout << "--- Test 2: Mixed columns (variant, variant_count, sickday, sickday_count, deadday) ---\n";
    vector<PopData::Column> cols2 = {
        PC::variant,
        PC::variant_count,
        PC::sickday,
        PC::sickday_count,
        PC::deadday
    };
    std::cout << "Row:\tvariant | variant_count | sickday | sickday_count | deadday |\n";
    std::cout << "--------------------------------------------------------------------\n";
    pop.print_table(rows, cols2);
    std::cout << "\n";

    // Test 3: Vaccine-related columns
    std::cout << "--- Test 3: Vaccine columns (vaxstatus, vaxrcvd, vax_count, quar, quarday) ---\n";
    vector<PopData::Column> cols3 = {
        PC::vaxstatus,
        PC::vaxrcvd,
        PC::vax_count,
        PC::quar,
        PC::quarday
    };
    std::cout << "Row:\tvaxstatus | vaxrcvd | vax_count | quar | quarday |\n";
    std::cout << "------------------------------------------------------------\n";
    pop.print_table(rows, cols3);
    std::cout << "\n";

    std::cout << "=== PopData Print Table Test Completed ===\n";
}

void test_age_distribution(const PopData& pop) {
    std::cout << "\n=== Testing Age Distribution ===\n\n";

    // Count people in each age group
    std::vector<int> age_counts(6, 0);  // 6 age groups (0=unknown, 1-5=actual groups)

    for (size_t i = 0; i < pop.popn; i++) {
        age_counts[pop.agegrp[i]]++;
    }

    // Display the distribution
    std::cout << "Population size: " << pop.popn << "\n\n";
    std::cout << "Age Group Distribution:\n";
    std::cout << "Age Group       | Count | Actual % | Expected % | Difference\n";
    std::cout << "----------------------------------------------------------------\n";

    // Expected percentages from AGE_DIST
    const std::vector<double> expected = {0.0, 0.251, 0.271, 0.255, 0.184, 0.039};

    for (size_t i = 0; i < age_counts.size(); i++) {
        double actual_pct = (pop.popn > 0) ? (100.0 * age_counts[i] / pop.popn) : 0.0;
        double expected_pct = expected[i] * 100.0;
        double diff = actual_pct - expected_pct;

        std::cout << std::setw(15) << std::left << pop.agegrp_lbl.to_str(i)
                  << " | " << std::setw(5) << std::right << age_counts[i]
                  << " | " << std::setw(8) << std::fixed << std::setprecision(2) << actual_pct << "%"
                  << " | " << std::setw(10) << expected_pct << "%"
                  << " | " << std::setw(6) << std::showpos << diff << "%\n" << std::noshowpos;
    }

    std::cout << "\nNote: Small differences are expected due to rounding when distributing\n";
    std::cout << "      " << pop.popn << " people across age groups.\n";

    // Verify total
    int total = std::accumulate(age_counts.begin(), age_counts.end(), 0);
    std::cout << "\nTotal count verification: " << total << " (should equal " << pop.popn << ")\n";

    std::cout << "\n=== Age Distribution Test Completed ===\n";
}

ModelParams test_model_params(ModelParams mp) {
  mp.geodata.print();
  cout << "\n";
  mp.variants.print();
  cout << "\n";
  mp.infectset.print();
  cout << "\n";
  mp.socialdata.print();
  cout << "\n";
  mp.progressionset.print(mp.variants);
  cout << "\n";
  mp.vaxset.print();
  cout << "\n";
  cout << "============ VaxList ==============\n";
  mp.vaxlist.print();
  cout << "============ End Vaxlist ==========\n";
  cout << "\n";
  mp.vaxsched.print();
  cout << "\n";

  return mp;
}

int main() {
  // tests
  // run_category_tests();

  // destructure ndays, day1, locale, dovax, mp, pop
  auto [ndays, day1, locale, dovax, mp, pop] = setup_sim(1000, 38015, "2020-01-01", false);

  test_popdata_print_table(pop);

  test_age_distribution(pop);

  cout << "\nday1: " << absl::FormatCivilTime(day1) << "\n";

  test_model_params(mp);

  return 0;
}