
#include "../src/lib_includes.h"

#include "../src/parameters.h"
#include "../src/setup.h"
#include "../src/sim.h"
#include "../src/population.h"
#include "../src/helpers.h"
#include "../src/random.h"
#include "../src/timing.h"
#include "../src/spread.h"

using std::string;
using std::vector;
using std::cout;


void run_category_tests() {
    std::cout << "=== Testing New Namespace Approach ===\n\n";

    // Test Status namespace
    std::cout << "--- Testing Status Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Status::none = " << static_cast<int>(Trait::Status("none")) << " (expected 0)\n";
    std::cout << "  Trait::Status::unexposed = " << static_cast<int>(Trait::Status("unexposed")) << " (expected 1)\n";
    std::cout << "  Status::infectious = " << static_cast<int>(Trait::Status("infectious")) << " (expected 2)\n";
    std::cout << "  Status::recovered = " << static_cast<int>(Trait::Status("recovered")) << " (expected 3)\n";
    std::cout << "  Status::dead = " << static_cast<int>(Trait::Status("dead")) << " (expected 4)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Status::to_str(0) = \"" << Trait::Status.to_str(0) << "\" (expected \"none\")\n";
    std::cout << "  Trait::Status::to_str(1) = \"" << Trait::Status.to_str(1) << "\" (expected \"unexposed\")\n";
    std::cout << "  Status::to_str(2) = \"" << Trait::Status.to_str(2) << "\" (expected \"infectious\")\n";
    std::cout << "  Status::to_str(3) = \"" << Trait::Status.to_str(3) << "\" (expected \"recovered\")\n";
    std::cout << "  Status::to_str(4) = \"" << Trait::Status.to_str(4) << "\" (expected \"dead\")\n";
    std::cout << "  Status::from_str(\"unexposed\") = " << static_cast<int>(Trait::Status("unexposed")) << " (expected 1)\n";
    std::cout << "  Status::from_str(\"infectious\") = " << static_cast<int>(Trait::Status("infectious")) << " (expected 2)\n";
    std::cout << "  Status::from_str(\"recovered\") = " << static_cast<int>(Trait::Status("recovered")) << " (expected 3)\n";
    std::cout << "  Status::from_str(\"dead\") = " << static_cast<int>(Trait::Status("dead")) << " (expected 4)\n";
    std::cout << "  Status::from_str(\"invalid\") = "
              << static_cast<int>(Trait::Status("invalid"))
              << " (expected 99 - default to none)\n";
    std::cout << "\n";

    // Test Condition namespace
    std::cout << "--- Testing Condition Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Condition::uninfected = " << static_cast<int>(Trait::Condition("uninfected")) << " (expected 0)\n";
    std::cout << "  Condition::nil = " << static_cast<int>(Trait::Condition("nil")) << " (expected 1)\n";
    std::cout << "  Condition::mild = " << static_cast<int>(Trait::Condition("mild")) << " (expected 2)\n";
    std::cout << "  Condition::sick = " << static_cast<int>(Trait::Condition("sick")) << " (expected 3)\n";
    std::cout << "  Condition::severe = " << static_cast<int>(Trait::Condition("severe")) << " (expected 4)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Condition::to_str(0) = \"" << Trait::Condition.to_str(0) << "\" (expected \"uninfected\")\n";
    std::cout << "  Condition::to_str(1) = \"" << Trait::Condition.to_str(1) << "\" (expected \"nil\")\n";
    std::cout << "  Condition::to_str(2) = \"" << Trait::Condition.to_str(2) << "\" (expected \"mild\")\n";
    std::cout << "  Condition::to_str(3) = \"" << Trait::Condition.to_str(3) << "\" (expected \"sick\")\n";
    std::cout << "  Condition::to_str(4) = \"" << Trait::Condition.to_str(4) << "\" (expected \"severe\")\n";
    std::cout << "  Condition::from_str(\"uninfected\") = " << static_cast<int>(Trait::Condition("uninfected")) << " (expected 0)\n";
    std::cout << "  Condition::from_str(\"nil\") = " << static_cast<int>(Trait::Condition("nil")) << " (expected 1)\n";
    std::cout << "  Condition::from_str(\"mild\") = " << static_cast<int>(Trait::Condition("mild")) << " (expected 2)\n";
    std::cout << "  Condition::from_str(\"sick\") = " << static_cast<int>(Trait::Condition("sick")) << " (expected 3)\n";
    std::cout << "  Condition::from_str(\"severe\") = " << static_cast<int>(Trait::Condition("severe")) << " (expected 4)\n";
    std::cout << "  Condition::from_str(\"invalid\") = "
              << static_cast<int>(Trait::Condition("invalid"))
              << " (expected 99 - default to uninfected)\n";
    std::cout << Trait::Condition.to_str(0) << " expected \"uninfected\"\n";
  
    std::cout << "\n";

    // Test Agegrp namespace
    std::cout << "--- Testing Agegrp Namespace ---\n";
    std::cout << "Enum values (0-indexed):\n";
    std::cout << "  Agegrp::unknown = " << static_cast<int>(Trait::Agegrp("unknown")) << " (expected 0)\n";
    std::cout << "  Agegrp::age0_19 = " << static_cast<int>(Trait::Agegrp("age0_19")) << " (expected 1)\n";
    std::cout << "  Agegrp::age20_39 = " << static_cast<int>(Trait::Agegrp("age20_39")) << " (expected 2)\n";
    std::cout << "  Agegrp::age40_59 = " << static_cast<int>(Trait::Agegrp("age40_59")) << " (expected 3)\n";
    std::cout << "  Agegrp::age60_79 = " << static_cast<int>(Trait::Agegrp("age60_79")) << " (expected 4)\n";
    std::cout << "  Agegrp::age80_up = " << static_cast<int>(Trait::Agegrp("age80_up")) << " (expected 5)\n";
    std::cout << "\nRound-trip tests:\n";
    std::cout << "  Agegrp::to_str(0) = \"" << Trait::Agegrp.to_str(0) << "\" (expected \"unknown\")\n";
    std::cout << "  Agegrp::to_str(1) = \"" << Trait::Agegrp.to_str(1) << "\" (expected \"age0_19\")\n";
    std::cout << "  Agegrp::to_str(2) = \"" << Trait::Agegrp.to_str(2) << "\" (expected \"age20_39\")\n";
    std::cout << "  Agegrp::to_str(3) = \"" << Trait::Agegrp.to_str(3) << "\" (expected \"age40_59\")\n";
    std::cout << "  Agegrp::to_str(4) = \"" << Trait::Agegrp.to_str(4) << "\" (expected \"age60_79\")\n";
    std::cout << "  Agegrp::to_str(5) = \"" << Trait::Agegrp.to_str(5) << "\" (expected \"age80_up\")\n";
    std::cout << "  Agegrp::from_str(\"age0_19\") = " << static_cast<int>(Trait::Agegrp("age0_19")) << " (expected 1)\n";
    std::cout << "  Agegrp::from_str(\"age20_39\") = " << static_cast<int>(Trait::Agegrp("age20_39")) << " (expected 2)\n";
    std::cout << "  Agegrp::from_str(\"age40_59\") = " << static_cast<int>(Trait::Agegrp("age40_59")) << " (expected 3)\n";
    std::cout << "  Agegrp::from_str(\"age60_79\") = " << static_cast<int>(Trait::Agegrp("age60_79")) << " (expected 4)\n";
    std::cout << "  Agegrp::from_str(\"age80_up\") = " << static_cast<int>(Trait::Agegrp("age80_up")) << " (expected 5)\n";
    std::cout << "  Agegrp::from_str(\"invalid\") = " << static_cast<int>(Trait::Agegrp("invalid")) << " (expected 99 - default to unknown)\n";
    std::cout << "\n";

    // Test boundary conditions
    std::cout << "--- Testing Boundary Conditions ---\n";
    std::cout << "  Status::to_str(99) = \"" << Trait::Status.to_str(99) << "\" (expected \"INVALID\" - out of range)\n";
    std::cout << "  Condition::to_str(99) = \"" << Trait::Condition.to_str(99) << "\" (expected \"INVALID\" - out of range)\n";
    std::cout << "  Agegrp::to_str(99) = \"" << Trait::Agegrp.to_str(99) << "\" (expected \"INVALID\" - out of range)\n";
    // std::cout << "  Vaccine::to_str(99) = \"" << Vaccine::to_str(99) << "\" (expected \"unknown\" - out of range)\n";
    std::cout << "\n";

    std::cout << "=== All namespace tests completed ===\n";
}

// Create a short alias for PopData::Column to use throughout this file
using PC = PopData::Column;

void test_popdata_print_table(PopData pop, vector<size_t> rows = {1, 25, 50, 75}) {
    rows.push_back(pop.popn);
  
    std::cout << "\n=== Testing PopData Print Table ===\n\n";
    std::cout << "Population size (popn): " << pop.popn << "\n";
    std::cout << "Vector size (popz = popn+1): " << pop.popz << "\n";
    std::cout << "Using 1-indexed rows (1 to popn), skipping row 0\n\n";

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

    for (size_t i = 1; i <= pop.popn; i++) {
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
  print_infectparams(mp.infectparams, mp.variants);
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

void test_popdata_size(Model model) {
  cout << "Vector sizing: " << model.pop.status.size() << " popz "
       << model.pop.popz << "\n";
  cout << "Index to actual population size: " << Trait::Agegrp.to_str(model.pop.agegrp[model.pop.popn])
       << "\n";
}

void test_multiple_infections() {
  std::cout << "\n=== Testing Multiple Infections Over Time ===\n\n";

  // Create a fresh model for this test
  std::cout << "Setting up independent test environment...\n";
  Model test_model = setup_sim(1000, 38015, "2020-01-01", false);
  PopData& pop = test_model.pop;
  RuntimeEnum& variants = test_model.mp.variants;

  // Find one person in age20_39 and one in age60_79
  size_t person_age20_39 = 0;
  size_t person_age60_79 = 0;

  for (size_t i = 1; i <= pop.popn; ++i) {
    if (person_age20_39 == 0 && pop.agegrp[i] == Trait::Age::age20_39) {
      person_age20_39 = i;
    }
    if (person_age60_79 == 0 && pop.agegrp[i] == Trait::Age::age60_79) {
      person_age60_79 = i;
    }
    if (person_age20_39 != 0 && person_age60_79 != 0) {
      break;
    }
  }

  if (person_age20_39 == 0 || person_age60_79 == 0) {
    std::cout << "ERROR: Could not find required age groups in population\n";
    return;
  }

  std::cout << "Testing with:\n";
  std::cout << "  Person " << person_age20_39 << " (age group: "
            << Trait::Agegrp.to_str(pop.agegrp[person_age20_39]) << ") - will be infected 10 times\n";
  std::cout << "  Person " << person_age60_79 << " (age group: "
            << Trait::Agegrp.to_str(pop.agegrp[person_age60_79]) << ") - will be infected 17 times\n\n";

  uint8_t base_variant = variants("base");
  uint8_t condition = Trait::Cond::nil;
  uint8_t duration = 5;

  // Reset sim day to 1
  sim::current_day = 1;

  // Infect person 1 ten times (days 1, 21, 41, 61, 81, 101, 121, 141, 161, 181)
  std::cout << "Infecting person " << person_age20_39 << " 10 times:\n";
  for (int infection = 0; infection < 10; ++infection) {
    int day = 1 + (infection * 20);
    sim::current_day = day;
    pop.make_sick(person_age20_39, base_variant, condition, duration);
    std::cout << "  Infection " << (infection + 1) << " on day " << day
              << " - variant_count: " << static_cast<int>(pop.variant_count[person_age20_39]) << "\n";
  }

  std::cout << "\nPerson " << person_age20_39 << " final state:\n";
  std::cout << "  variant_count: " << static_cast<int>(pop.variant_count[person_age20_39]) << "\n";
  std::cout << "  Infection history (variant, sickday):\n";
  int count1 = std::min(static_cast<int>(pop.variant_count[person_age20_39]), 16);
  for (int i = 0; i < count1; ++i) {
    std::cout << "    [" << i << "] variant: " << variants.to_str(pop.variant[person_age20_39][i])
              << ", sickday: " << pop.sickday[person_age20_39][i] << "\n";
  }

  // Infect person 2 seventeen times (days 1, 21, 41, ..., 321)
  std::cout << "\n\nInfecting person " << person_age60_79 << " 17 times:\n";
  for (int infection = 0; infection < 17; ++infection) {
    int day = 1 + (infection * 20);
    sim::current_day = day;
    pop.make_sick(person_age60_79, base_variant, condition, duration);
    std::cout << "  Infection " << (infection + 1) << " on day " << day
              << " - variant_count: " << static_cast<int>(pop.variant_count[person_age60_79]) << "\n";
  }

  std::cout << "\nPerson " << person_age60_79 << " final state:\n";
  std::cout << "  variant_count: " << static_cast<int>(pop.variant_count[person_age60_79]) << "\n";
  std::cout << "  Infection history (variant, sickday):\n";
  int count2 = std::min(static_cast<int>(pop.variant_count[person_age60_79]), 16);
  std::cout << "  (Showing " << count2 << " most recent infections, max capacity is 16)\n";
  for (int i = 0; i < count2; ++i) {
    std::cout << "    [" << i << "] variant: " << variants.to_str(pop.variant[person_age60_79][i])
              << ", sickday: " << pop.sickday[person_age60_79][i] << "\n";
  }

  std::cout << "\n=== Multiple Infections Test Completed ===\n";
}

void test_seedcase_multiple_infections() {
  std::cout << "\n=== Testing SeedCase with Multiple Infections ===\n\n";

  // Create a fresh model for this test
  std::cout << "Setting up independent test environment...\n";
  Model test_model = setup_sim(1000, 38015, "2020-01-01", false);
  PopData& pop = test_model.pop;
  RuntimeEnum& variants = test_model.mp.variants;

  uint8_t base_variant = variants("base");
  uint8_t condition = Trait::Cond::nil;
  uint8_t duration = 5;

  // Create SeedCases:
  // - One for age20_39 that triggers 10 times (days 1, 21, 41, ..., 181)
  // - One for age60_79 that triggers 17 times (days 1, 21, 41, ..., 321)

  std::vector<SeedCase> seed_cases;

  // Create 10 seed cases for age20_39 (one person each time)
  for (int i = 0; i < 10; ++i) {
    int trigger_day = 1 + (i * 20);
    std::vector<SeedFilter> filters{{Trait::Age::age20_39, condition, 0, base_variant, 1}};
    seed_cases.emplace_back(trigger_day, true, filters, pop);
  }

  // Create 17 seed cases for age60_79 (one person each time)
  for (int i = 0; i < 17; ++i) {
    int trigger_day = 1 + (i * 20);
    std::vector<SeedFilter> filters{{Trait::Age::age60_79, condition, 0, base_variant, 1}};
    seed_cases.emplace_back(trigger_day, true, filters, pop);
  }

  std::cout << "Created " << seed_cases.size() << " seed cases\n";
  std::cout << "  10 for age20_39 (days 1, 21, 41, ..., 181)\n";
  std::cout << "  17 for age60_79 (days 1, 21, 41, ..., 321)\n\n";

  // Track which persons get infected
  size_t person_age20_39 = 0;
  size_t person_age60_79 = 0;

  // Simulate days 1 through 321, triggering seed cases as appropriate
  std::cout << "Running simulation through day 321...\n";
  for (int day = 1; day <= 321; ++day) {
    sim::current_day = day;

    // Check each seed case to see if it should trigger today
    for (auto& seed_case : seed_cases) {
      if (seed_case.triggerday == day) {
        auto seeded = seed_case();

        // Track which persons were seeded
        if (!seeded.empty()) {
          size_t person = seeded[0];
          if (pop.agegrp[person] == Trait::Age::age20_39) {
            person_age20_39 = person;
            std::cout << "  Day " << day << ": Infected person " << person
                      << " (age20_39), variant_count now: "
                      << static_cast<int>(pop.variant_count[person]) << "\n";
          } else if (pop.agegrp[person] == Trait::Age::age60_79) {
            person_age60_79 = person;
            std::cout << "  Day " << day << ": Infected person " << person
                      << " (age60_79), variant_count now: "
                      << static_cast<int>(pop.variant_count[person]) << "\n";
          }
        }
      }
    }
  }

  // Display results
  if (person_age20_39 == 0 || person_age60_79 == 0) {
    std::cout << "\nERROR: Not all persons were infected\n";
    return;
  }

  std::cout << "\n=== Final Results ===\n\n";

  std::cout << "Person " << person_age20_39 << " (age20_39) final state:\n";
  std::cout << "  variant_count: " << static_cast<int>(pop.variant_count[person_age20_39]) << "\n";
  std::cout << "  Infection history (variant, sickday):\n";
  int count1 = std::min(static_cast<int>(pop.variant_count[person_age20_39]), 16);
  for (int i = 0; i < count1; ++i) {
    std::cout << "    [" << i << "] variant: " << variants.to_str(pop.variant[person_age20_39][i])
              << ", sickday: " << pop.sickday[person_age20_39][i] << "\n";
  }

  std::cout << "\nPerson " << person_age60_79 << " (age60_79) final state:\n";
  std::cout << "  variant_count: " << static_cast<int>(pop.variant_count[person_age60_79]) << "\n";
  std::cout << "  Infection history (variant, sickday):\n";
  int count2 = std::min(static_cast<int>(pop.variant_count[person_age60_79]), 16);
  std::cout << "  (Showing " << count2 << " most recent infections, max capacity is 16)\n";
  for (int i = 0; i < count2; ++i) {
    std::cout << "    [" << i << "] variant: " << variants.to_str(pop.variant[person_age60_79][i])
              << ", sickday: " << pop.sickday[person_age60_79][i] << "\n";
  }

  std::cout << "\n=== SeedCase Multiple Infections Test Completed ===\n";
}

void test_seeding_and_spread() {
  std::cout << "\n=== Integration Test: Seeding and Spread Contact Generation ===\n\n";

  // Setup simulation - this creates the model with real parameters
  std::cout << "Setting up simulation with locale 38015, for 3 days...\n";
  Model model = setup_sim(3, 38015, "2020-01-01", false);

  std::cout << "Population size: " << model.pop.popn << "\n";
  std::cout << "Number of days: " << model.ndays << "\n";
  std::cout << "Start date: " << absl::FormatCivilTime(model.day1) << "\n\n";

  std::cout << "--- Running Simulation ---\n";
  std::cout << "This will use the SeedCases defined in sim.cpp\n";
  std::cout << "Expected: 6 people infected (3 age20_39 + 3 age40_59) with duration=5\n";
  std::cout << "The spread() function will print contacts for each infectious person\n\n";

  // Run the actual simulation - this uses the real SeedCases from sim.cpp
  runsim(model);

  std::cout << "\n=== Integration Test Completed ===\n";
}

void test_random_functions() {
  std::cout << "\n=== Testing xo::random Functions ===\n\n";

  // Seed the RNG for reproducible results
  xo::seed(12345);

  // Test get_n_draws with min=1, max=100001, n=7
  std::cout << "--- Testing xo::get_n_draws ---\n";
  std::cout << "Parameters: min=1, max=100001, n=7\n\n";

  Timing timer;
  timer.start();
  vector<size_t> result_vec(250);
  xo::get_n_draws<size_t>(1, 100001, 7, result_vec);
  timer.cum();

  std::cout << "Generated " << result_vec.size() << " random integers in "
            << timer.show() << " seconds\n";
  for (size_t i = 0; i < result_vec.size(); ++i) {
    std::cout << "  [" << i << "] = " << result_vec[i] << "\n";
  }

  // Verify the results
  std::cout << "\n--- Verification ---\n";
  bool all_valid = true;
  bool correct_count = (result_vec.size() == 7);

  std::cout << "Count check: " << (correct_count ? "PASS" : "FAIL")
            << " (expected 7, got " << result_vec.size() << ")\n";

  for (size_t i = 0; i < result_vec.size(); ++i) {
    bool in_range = (result_vec[i] >= 1 && result_vec[i] <= 100001);
    if (!in_range) {
      all_valid = false;
      std::cout << "  Value [" << i << "] = " << result_vec[i]
                << " is OUT OF RANGE [1, 100001]\n";
    }
  }

  if (all_valid) {
    std::cout << "Range check: PASS (all values in [1, 100001])\n";
  } else {
    std::cout << "Range check: FAIL (some values out of range)\n";
  }

  // Test other random functions
  std::cout << "\n--- Testing other xo functions ---\n";

  // Test xo::get
  int single_draw = xo::get(1, 100);
  std::cout << "xo::get(1, 100) = " << single_draw
            << " (should be in [1, 100]): "
            << ((single_draw >= 1 && single_draw <= 100) ? "PASS" : "FAIL") << "\n";

  // Test xo::gamma_int with shape=1.0, scale=1.4
  int gamma_val = xo::gamma_int(1.0, 1.4, 12);
  std::cout << "xo::gamma_int(1.0, 1.4, 12) = " << gamma_val
            << " (should be in [0, 12]): "
            << ((gamma_val >= 0 && gamma_val <= 12) ? "PASS" : "FAIL") << "\n";

  // Test xo::bernoulli
  int bern_val = xo::bernoulli(0.5);
  std::cout << "xo::bernoulli(0.5) = " << bern_val
            << " (should be 0 or 1): "
            << ((bern_val == 0 || bern_val == 1) ? "PASS" : "FAIL") << "\n";

  // Test xo::categorical_uniform
  int cat_val = xo::categorical_uniform(5);
  std::cout << "xo::categorical_uniform(5) = " << cat_val
            << " (should be in [0, 4]): "
            << ((cat_val >= 0 && cat_val <= 4) ? "PASS" : "FAIL") << "\n";

  // Performance test with timing
  std::cout << "\n--- Performance Testing with Timing class ---\n";

  // Test 1: Generate many small draws
  timer.reset();
  timer.start();
  for (int i = 0; i < 10000; ++i) {
    xo::get(1, 100);
  }
  timer.cum();
  std::cout << "10,000 calls to xo::get(1, 100): " << timer.show() << " seconds\n";

  // Test 2: Generate large batch
  timer.reset();
  timer.start();
  vector<size_t> large_batch(10000);
  xo::get_n_draws<size_t>(1, 100001, 10000, large_batch);
  timer.cum();
  std::cout << "Single call to xo::get_n_draws(1, 100001, 10000): "
            << timer.show() << " seconds\n";
  std::cout << "  (Generated " << large_batch.size() << " values)\n";

  // Test 3: Multiple cumulative timing
  vector<size_t> buffer(1000);
  timer.reset();
  std::cout << "\nTesting cumulative timing with multiple segments:\n";

  timer.start();
  xo::get_n_draws<size_t>(1, 1000, 1000, buffer);
  timer.cum();
  std::cout << "  After segment 1: " << timer.show() << " seconds\n";

  timer.start();
  xo::get_n_draws<size_t>(1, 1000, 1000, buffer);
  timer.cum();
  std::cout << "  After segment 2: " << timer.show() << " seconds (cumulative)\n";

  timer.start();
  xo::get_n_draws<size_t>(1, 1000, 1000, buffer);
  timer.cum();
  std::cout << "  After segment 3: " << timer.show() << " seconds (cumulative)\n";

  std::cout << "\n=== Random Functions Test Completed ===\n";
}


int main() {
  // Test seeding and spread contact generation
  // test_seeding_and_spread();

  // Test random number generator functions
  // test_random_functions();

  // tests
  // run_category_tests();

  // Test model params
  // Model model = setup_sim(1000, 38015, "2020-01-01", true);
  // test_model_params(model.mp);

  // Test spread function with 180-day simulation
  std::cout << "\n=== Testing Spread Function (180-day simulation) ===\n\n";
  Model model = setup_sim(180, 38015, "2020-01-01", false);
  std::cout << "Population: " << model.pop.popn << "\n";
  std::cout << "Running simulation for " << model.ndays << " days...\n\n";

  runsim(model);

  // Count occurrences of infected and recovered--> gut check
    // auto count = std::count(model.pop.status.begin(), model.pop.status.end(), Trait::Stat::recovered);
    // std::cout << "Count of recovered: " << count << '\n';
    // count = std::count_if(model.pop.variant_count.begin(), model.pop.variant_count.end(),
    //                            [](int x) { return x > 0; });
    // std::cout << "Count of variant_count > 0: " << count << '\n';

  std::cout << "\n=== Simulation Complete ===\n";

  return 0;
}