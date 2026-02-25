
#include "../src/lib_includes.h"

#include "../src/parameters.h"
#include "../src/setup.h"
#include "../src/sim.h"
#include "../src/population.h"
#include "../src/helpers.h"
#include "../src/random.h"
#include "../src/timing.h"
#include "../src/spread.h"
#include "../src/progression.h"

using std::string;
using std::vector;


void run_category_tests() {
    fmt::print("=== Testing New Namespace Approach ===\n\n");

    // Test Status namespace
    fmt::println("--- Testing Status Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Status::none = {} (expected 0)", static_cast<int>(Trait::Status("none")));
    fmt::println("  Trait::Status::unexposed = {} (expected 1)", static_cast<int>(Trait::Status("unexposed")));
    fmt::println("  Status::infectious = {} (expected 2)", static_cast<int>(Trait::Status("infectious")));
    fmt::println("  Status::recovered = {} (expected 3)", static_cast<int>(Trait::Status("recovered")));
    fmt::println("  Status::dead = {} (expected 4)", static_cast<int>(Trait::Status("dead")));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Status::to_str(0) = \"{}\" (expected \"none\")", Trait::Status.to_str(0));
    fmt::println("  Trait::Status::to_str(1) = \"{}\" (expected \"unexposed\")", Trait::Status.to_str(1));
    fmt::println("  Status::to_str(2) = \"{}\" (expected \"infectious\")", Trait::Status.to_str(2));
    fmt::println("  Status::to_str(3) = \"{}\" (expected \"recovered\")", Trait::Status.to_str(3));
    fmt::println("  Status::to_str(4) = \"{}\" (expected \"dead\")", Trait::Status.to_str(4));
    fmt::println("  Status::from_str(\"unexposed\") = {} (expected 1)", static_cast<int>(Trait::Status("unexposed")));
    fmt::println("  Status::from_str(\"infectious\") = {} (expected 2)", static_cast<int>(Trait::Status("infectious")));
    fmt::println("  Status::from_str(\"recovered\") = {} (expected 3)", static_cast<int>(Trait::Status("recovered")));
    fmt::println("  Status::from_str(\"dead\") = {} (expected 4)", static_cast<int>(Trait::Status("dead")));
    fmt::println("  Status::from_str(\"invalid\") = {} (expected 99 - default to none)",
                 static_cast<int>(Trait::Status("invalid")));
    fmt::print("\n");

    // Test Condition namespace
    fmt::println("--- Testing Condition Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Condition::uninfected = {} (expected 0)", static_cast<int>(Trait::Condition("uninfected")));
    fmt::println("  Condition::nil = {} (expected 1)", static_cast<int>(Trait::Condition("nil")));
    fmt::println("  Condition::mild = {} (expected 2)", static_cast<int>(Trait::Condition("mild")));
    fmt::println("  Condition::sick = {} (expected 3)", static_cast<int>(Trait::Condition("sick")));
    fmt::println("  Condition::severe = {} (expected 4)", static_cast<int>(Trait::Condition("severe")));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Condition::to_str(0) = \"{}\" (expected \"uninfected\")", Trait::Condition.to_str(0));
    fmt::println("  Condition::to_str(1) = \"{}\" (expected \"nil\")", Trait::Condition.to_str(1));
    fmt::println("  Condition::to_str(2) = \"{}\" (expected \"mild\")", Trait::Condition.to_str(2));
    fmt::println("  Condition::to_str(3) = \"{}\" (expected \"sick\")", Trait::Condition.to_str(3));
    fmt::println("  Condition::to_str(4) = \"{}\" (expected \"severe\")", Trait::Condition.to_str(4));
    fmt::println("  Condition::from_str(\"uninfected\") = {} (expected 0)", static_cast<int>(Trait::Condition("uninfected")));
    fmt::println("  Condition::from_str(\"nil\") = {} (expected 1)", static_cast<int>(Trait::Condition("nil")));
    fmt::println("  Condition::from_str(\"mild\") = {} (expected 2)", static_cast<int>(Trait::Condition("mild")));
    fmt::println("  Condition::from_str(\"sick\") = {} (expected 3)", static_cast<int>(Trait::Condition("sick")));
    fmt::println("  Condition::from_str(\"severe\") = {} (expected 4)", static_cast<int>(Trait::Condition("severe")));
    fmt::println("  Condition::from_str(\"invalid\") = {} (expected 99 - default to uninfected)",
                 static_cast<int>(Trait::Condition("invalid")));
    fmt::println("{} expected \"uninfected\"", Trait::Condition.to_str(0));

    fmt::print("\n");

    // Test Agegrp namespace
    fmt::println("--- Testing Agegrp Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Agegrp::unknown = {} (expected 0)", static_cast<int>(Trait::Agegrp("unknown")));
    fmt::println("  Agegrp::age0_19 = {} (expected 1)", static_cast<int>(Trait::Agegrp("age0_19")));
    fmt::println("  Agegrp::age20_39 = {} (expected 2)", static_cast<int>(Trait::Agegrp("age20_39")));
    fmt::println("  Agegrp::age40_59 = {} (expected 3)", static_cast<int>(Trait::Agegrp("age40_59")));
    fmt::println("  Agegrp::age60_79 = {} (expected 4)", static_cast<int>(Trait::Agegrp("age60_79")));
    fmt::println("  Agegrp::age80_up = {} (expected 5)", static_cast<int>(Trait::Agegrp("age80_up")));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Agegrp::to_str(0) = \"{}\" (expected \"unknown\")", Trait::Agegrp.to_str(0));
    fmt::println("  Agegrp::to_str(1) = \"{}\" (expected \"age0_19\")", Trait::Agegrp.to_str(1));
    fmt::println("  Agegrp::to_str(2) = \"{}\" (expected \"age20_39\")", Trait::Agegrp.to_str(2));
    fmt::println("  Agegrp::to_str(3) = \"{}\" (expected \"age40_59\")", Trait::Agegrp.to_str(3));
    fmt::println("  Agegrp::to_str(4) = \"{}\" (expected \"age60_79\")", Trait::Agegrp.to_str(4));
    fmt::println("  Agegrp::to_str(5) = \"{}\" (expected \"age80_up\")", Trait::Agegrp.to_str(5));
    fmt::println("  Agegrp::from_str(\"age0_19\") = {} (expected 1)", static_cast<int>(Trait::Agegrp("age0_19")));
    fmt::println("  Agegrp::from_str(\"age20_39\") = {} (expected 2)", static_cast<int>(Trait::Agegrp("age20_39")));
    fmt::println("  Agegrp::from_str(\"age40_59\") = {} (expected 3)", static_cast<int>(Trait::Agegrp("age40_59")));
    fmt::println("  Agegrp::from_str(\"age60_79\") = {} (expected 4)", static_cast<int>(Trait::Agegrp("age60_79")));
    fmt::println("  Agegrp::from_str(\"age80_up\") = {} (expected 5)", static_cast<int>(Trait::Agegrp("age80_up")));
    fmt::println("  Agegrp::from_str(\"invalid\") = {} (expected 99 - default to unknown)", static_cast<int>(Trait::Agegrp("invalid")));
    fmt::print("\n");

    // Test boundary conditions
    fmt::println("--- Testing Boundary Conditions ---");
    fmt::println("  Status::to_str(99) = \"{}\" (expected \"INVALID\" - out of range)", Trait::Status.to_str(99));
    fmt::println("  Condition::to_str(99) = \"{}\" (expected \"INVALID\" - out of range)", Trait::Condition.to_str(99));
    fmt::println("  Agegrp::to_str(99) = \"{}\" (expected \"INVALID\" - out of range)", Trait::Agegrp.to_str(99));
    // fmt::println("  Vaccine::to_str(99) = \"{}\" (expected \"unknown\" - out of range)", Vaccine::to_str(99));
    fmt::print("\n");

    fmt::println("=== All namespace tests completed ===");
}

// Create a short alias for PopData::Column to use throughout this file
using PC = PopData::Column;

void test_popdata_print_table(PopData pop, vector<size_t> rows = {1, 25, 50, 75}) {
    rows.push_back(pop.popn);

    fmt::print("\n=== Testing PopData Print Table ===\n\n");
    fmt::println("Population size (popn): {}", pop.popn);
    fmt::println("Vector size (popz = popn+1): {}", pop.popz);
    fmt::println("Using 1-indexed rows (1 to popn), skipping row 0\n");

    // Test 1: First 5 columns (simple uint8_t vectors)
    fmt::println("--- Test 1: Simple columns (status, agegrp, cond, duration, ring) ---");
    vector<PopData::Column> cols1 = {
        PC::status,
        PC::agegrp,
        PC::cond,
        PC::duration,
        PC::ring
    };
    fmt::println("Row:\tstatus | agegrp | cond | duration | ring |");
    fmt::println("------------------------------------------------------------");
    pop.print_table(rows, cols1);
    fmt::print("\n");

    // Test 2: Mix of simple and array columns
    fmt::println("--- Test 2: Mixed columns (variant, variant_count, sickday, sickday_count, deadday) ---");
    vector<PopData::Column> cols2 = {
        PC::variant,
        PC::variant_count,
        PC::sickday,
        PC::sickday_count,
        PC::deadday
    };
    fmt::println("Row:\tvariant | variant_count | sickday | sickday_count | deadday |");
    fmt::println("--------------------------------------------------------------------");
    pop.print_table(rows, cols2);
    fmt::print("\n");

    // Test 3: Vaccine-related columns
    fmt::println("--- Test 3: Vaccine columns (vaxstatus, vaxrcvd, vax_count, quar, quarday) ---");
    vector<PopData::Column> cols3 = {
        PC::vaxstatus,
        PC::vaxrcvd,
        PC::vax_count,
        PC::quar,
        PC::quarday
    };
    fmt::println("Row:\tvaxstatus | vaxrcvd | vax_count | quar | quarday |");
    fmt::println("------------------------------------------------------------");
    pop.print_table(rows, cols3);
    fmt::print("\n");

    fmt::println("=== PopData Print Table Test Completed ===");
}

void test_age_distribution(const PopData& pop) {
    fmt::print("\n=== Testing Age Distribution ===\n\n");

    // Count people in each age group
    std::vector<int> age_counts(6, 0);  // 6 age groups (0=unknown, 1-5=actual groups)

    for (size_t i = 1; i <= pop.popn; ++i) {
        ++age_counts[pop.agegrp[i]];
    }

    // Display the distribution
    fmt::println("Population size: {}\n", pop.popn);
    fmt::println("Age Group Distribution:");
    fmt::println("Age Group       | Count | Actual % | Expected % | Difference");
    fmt::println("----------------------------------------------------------------");

    // Expected percentages from AGE_DIST
    const std::vector<double> expected = {0.0, 0.251, 0.271, 0.255, 0.184, 0.039};

    for (size_t i = 0; i < age_counts.size(); ++i) {
        double actual_pct = (pop.popn > 0) ? (100.0 * age_counts[i] / pop.popn) : 0.0;
        double expected_pct = expected[i] * 100.0;
        double diff = actual_pct - expected_pct;

        fmt::println("{:<15} | {:>5} | {:>8.2f}% | {:>10.2f}% | {:>+6.2f}%",
                     pop.agegrp_lbl.to_str(i), age_counts[i], actual_pct, expected_pct, diff);
    }

    fmt::println("\nNote: Small differences are expected due to rounding when distributing");
    fmt::println("      {} people across age groups.", pop.popn);

    // Verify total
    int total = std::accumulate(age_counts.begin(), age_counts.end(), 0);
    fmt::println("\nTotal count verification: {} (should equal {})", total, pop.popn);

    fmt::println("\n=== Age Distribution Test Completed ===");
}

ModelParams test_model_params(ModelParams mp) {
  mp.geodata.print();
  fmt::print("\n");
  mp.variants.print();
  fmt::print("\n");
  print_infectparams(mp.infectparams, mp.variants);
  fmt::print("\n");
  mp.socialdata.print();
  fmt::print("\n");
  mp.progressionset.print(mp.variants);
  fmt::print("\n");
  mp.vaxset.print();
  fmt::print("\n");
  fmt::println("============ VaxList ==============");
  mp.vaxlist.print();
  fmt::println("============ End Vaxlist ==========");
  fmt::print("\n");
  mp.vaxsched.print();
  fmt::print("\n");

  return mp;
}

void test_popdata_size(Model model) {
  fmt::println("Vector sizing: {} popz {}", model.pop.status.size(), model.pop.popz);
  fmt::println("Index to actual population size: {}",
               Trait::Agegrp.to_str(model.pop.agegrp[model.pop.popn]));
}

void test_multiple_infections() {
  fmt::print("\n=== Testing Multiple Infections Over Time ===\n\n");

  // Create a fresh model for this test
  fmt::println("Setting up independent test environment...");
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
    fmt::println("ERROR: Could not find required age groups in population");
    return;
  }

  fmt::println("Testing with:");
  fmt::println("  Person {} (age group: {}) - will be infected 10 times",
               person_age20_39, Trait::Agegrp.to_str(pop.agegrp[person_age20_39]));
  fmt::println("  Person {} (age group: {}) - will be infected 17 times\n",
               person_age60_79, Trait::Agegrp.to_str(pop.agegrp[person_age60_79]));

  uint8_t base_variant = variants("base");
  uint8_t condition = Trait::Cond::nil;
  uint8_t duration = 5;

  // Reset sim day to 1
  sim::current_day = 1;

  // Infect person 1 ten times (days 1, 21, 41, 61, 81, 101, 121, 141, 161, 181)
  fmt::println("Infecting person {} 10 times:", person_age20_39);
  for (int infection = 0; infection < 10; ++infection) {
    int day = 1 + (infection * 20);
    sim::current_day = day;
    pop.make_sick(person_age20_39, base_variant, condition, duration);
    fmt::println("  Infection {} on day {} - variant_count: {}",
                 infection + 1, day, static_cast<int>(pop.variant_count[person_age20_39]));
  }

  fmt::println("\nPerson {} final state:", person_age20_39);
  fmt::println("  variant_count: {}", static_cast<int>(pop.variant_count[person_age20_39]));
  fmt::println("  Infection history (variant, sickday):");
  int count1 = std::min(static_cast<int>(pop.variant_count[person_age20_39]), 16);
  for (int i = 0; i < count1; ++i) {
    fmt::println("    [{}] variant: {}, sickday: {}",
                 i, variants.to_str(pop.variant[person_age20_39][i]), pop.sickday[person_age20_39][i]);
  }

  // Infect person 2 seventeen times (days 1, 21, 41, ..., 321)
  fmt::print("\n\n");
  fmt::println("Infecting person {} 17 times:", person_age60_79);
  for (int infection = 0; infection < 17; ++infection) {
    int day = 1 + (infection * 20);
    sim::current_day = day;
    pop.make_sick(person_age60_79, base_variant, condition, duration);
    fmt::println("  Infection {} on day {} - variant_count: {}",
                 infection + 1, day, static_cast<int>(pop.variant_count[person_age60_79]));
  }

  fmt::println("\nPerson {} final state:", person_age60_79);
  fmt::println("  variant_count: {}", static_cast<int>(pop.variant_count[person_age60_79]));
  fmt::println("  Infection history (variant, sickday):");
  int count2 = std::min(static_cast<int>(pop.variant_count[person_age60_79]), 16);
  fmt::println("  (Showing {} most recent infections, max capacity is 16)", count2);
  for (int i = 0; i < count2; ++i) {
    fmt::println("    [{}] variant: {}, sickday: {}",
                 i, variants.to_str(pop.variant[person_age60_79][i]), pop.sickday[person_age60_79][i]);
  }

  fmt::println("\n=== Multiple Infections Test Completed ===");
}

void test_seedcase_multiple_infections() {
  fmt::print("\n=== Testing SeedCase with Multiple Infections ===\n\n");

  // Create a fresh model for this test
  fmt::println("Setting up independent test environment...");
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

  fmt::println("Created {} seed cases", seed_cases.size());
  fmt::println("  10 for age20_39 (days 1, 21, 41, ..., 181)");
  fmt::println("  17 for age60_79 (days 1, 21, 41, ..., 321)\n");

  // Track which persons get infected
  size_t person_age20_39 = 0;
  size_t person_age60_79 = 0;

  // Simulate days 1 through 321, triggering seed cases as appropriate
  fmt::println("Running simulation through day 321...");
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
            fmt::println("  Day {}: Infected person {} (age20_39), variant_count now: {}",
                         day, person, static_cast<int>(pop.variant_count[person]));
          } else if (pop.agegrp[person] == Trait::Age::age60_79) {
            person_age60_79 = person;
            fmt::println("  Day {}: Infected person {} (age60_79), variant_count now: {}",
                         day, person, static_cast<int>(pop.variant_count[person]));
          }
        }
      }
    }
  }

  // Display results
  if (person_age20_39 == 0 || person_age60_79 == 0) {
    fmt::println("\nERROR: Not all persons were infected");
    return;
  }

  fmt::print("\n=== Final Results ===\n\n");

  fmt::println("Person {} (age20_39) final state:", person_age20_39);
  fmt::println("  variant_count: {}", static_cast<int>(pop.variant_count[person_age20_39]));
  fmt::println("  Infection history (variant, sickday):");
  int count1 = std::min(static_cast<int>(pop.variant_count[person_age20_39]), 16);
  for (int i = 0; i < count1; ++i) {
    fmt::println("    [{}] variant: {}, sickday: {}",
                 i, variants.to_str(pop.variant[person_age20_39][i]), pop.sickday[person_age20_39][i]);
  }

  fmt::println("\nPerson {} (age60_79) final state:", person_age60_79);
  fmt::println("  variant_count: {}", static_cast<int>(pop.variant_count[person_age60_79]));
  fmt::println("  Infection history (variant, sickday):");
  int count2 = std::min(static_cast<int>(pop.variant_count[person_age60_79]), 16);
  fmt::println("  (Showing {} most recent infections, max capacity is 16)", count2);
  for (int i = 0; i < count2; ++i) {
    fmt::println("    [{}] variant: {}, sickday: {}",
                 i, variants.to_str(pop.variant[person_age60_79][i]), pop.sickday[person_age60_79][i]);
  }

  fmt::println("\n=== SeedCase Multiple Infections Test Completed ===");
}

void test_seeding_and_spread() {
  fmt::print("\n=== Integration Test: Seeding and Spread Contact Generation ===\n\n");

  // Setup simulation - this creates the model with real parameters
  fmt::println("Setting up simulation with locale 38015, for 3 days...");
  Model model = setup_sim(3, 38015, "2020-01-01", false);

  fmt::println("Population size: {}", model.pop.popn);
  fmt::println("Number of days: {}", model.ndays);
  fmt::println("Start date: {}\n", absl::FormatCivilTime(model.day1));

  fmt::println("--- Running Simulation ---");
  fmt::println("This will use the SeedCases defined in sim.cpp");
  fmt::println("Expected: 6 people infected (3 age20_39 + 3 age40_59) with duration=5");
  fmt::println("The spread() function will print contacts for each infectious person\n");

  // Run the actual simulation - this uses the real SeedCases from sim.cpp
  runsim(model);

  fmt::println("\n=== Integration Test Completed ===");
}

void test_random_functions() {
  fmt::print("\n=== Testing xo::random Functions ===\n\n");

  // Seed the RNG for reproducible results
  xo::seed(12345);

  // Test get_n_draws with min=1, max=100001, n=7
  fmt::println("--- Testing xo::get_n_draws ---");
  fmt::println("Parameters: min=1, max=100001, n=7\n");

  Timing timer;
  timer.start();
  vector<size_t> result_vec(250);
  xo::get_n_draws<size_t>(1, 100001, 7, result_vec);
  timer.cum();

  fmt::println("Generated {} random integers in {} seconds", result_vec.size(), timer.show());
  for (size_t i = 0; i < result_vec.size(); ++i) {
    fmt::println("  [{}] = {}", i, result_vec[i]);
  }

  // Verify the results
  fmt::println("\n--- Verification ---");
  bool all_valid = true;
  bool correct_count = (result_vec.size() == 7);

  fmt::println("Count check: {} (expected 7, got {})",
               correct_count ? "PASS" : "FAIL", result_vec.size());

  for (size_t i = 0; i < result_vec.size(); ++i) {
    bool in_range = (result_vec[i] >= 1 && result_vec[i] <= 100001);
    if (!in_range) {
      all_valid = false;
      fmt::println("  Value [{}] = {} is OUT OF RANGE [1, 100001]", i, result_vec[i]);
    }
  }

  if (all_valid) {
    fmt::println("Range check: PASS (all values in [1, 100001])");
  } else {
    fmt::println("Range check: FAIL (some values out of range)");
  }

  // Test other random functions
  fmt::println("\n--- Testing other xo functions ---");

  // Test xo::get
  int single_draw = xo::get(1, 100);
  fmt::println("xo::get(1, 100) = {} (should be in [1, 100]): {}",
               single_draw, (single_draw >= 1 && single_draw <= 100) ? "PASS" : "FAIL");

  // Test xo::gamma_int with shape=1.0, scale=1.4
  int gamma_val = xo::gamma_int(1.0, 1.4, 12);
  fmt::println("xo::gamma_int(1.0, 1.4, 12) = {} (should be in [0, 12]): {}",
               gamma_val, (gamma_val >= 0 && gamma_val <= 12) ? "PASS" : "FAIL");

  // Test xo::bernoulli
  int bern_val = xo::bernoulli(0.5);
  fmt::println("xo::bernoulli(0.5) = {} (should be 0 or 1): {}",
               bern_val, (bern_val == 0 || bern_val == 1) ? "PASS" : "FAIL");

  // Test xo::categorical_uniform
  int cat_val = xo::categorical_uniform(5);
  fmt::println("xo::categorical_uniform(5) = {} (should be in [0, 4]): {}",
               cat_val, (cat_val >= 0 && cat_val <= 4) ? "PASS" : "FAIL");

  // Performance test with timing
  fmt::println("\n--- Performance Testing with Timing class ---");

  // Test 1: Generate many small draws
  timer.reset();
  timer.start();
  for (int i = 0; i < 10000; ++i) {
    xo::get(1, 100);
  }
  timer.cum();
  fmt::println("10,000 calls to xo::get(1, 100): {} seconds", timer.show());

  // Test 2: Generate large batch
  timer.reset();
  timer.start();
  vector<size_t> large_batch(10000);
  xo::get_n_draws<size_t>(1, 100001, 10000, large_batch);
  timer.cum();
  fmt::println("Single call to xo::get_n_draws(1, 100001, 10000): {} seconds", timer.show());
  fmt::println("  (Generated {} values)", large_batch.size());

  // Test 3: Multiple cumulative timing
  vector<size_t> buffer(1000);
  timer.reset();
  fmt::println("\nTesting cumulative timing with multiple segments:");

  timer.start();
  xo::get_n_draws<size_t>(1, 1000, 1000, buffer);
  timer.cum();
  fmt::println("  After segment 1: {} seconds", timer.show());

  timer.start();
  xo::get_n_draws<size_t>(1, 1000, 1000, buffer);
  timer.cum();
  fmt::println("  After segment 2: {} seconds (cumulative)", timer.show());

  timer.start();
  xo::get_n_draws<size_t>(1, 1000, 1000, buffer);
  timer.cum();
  fmt::println("  After segment 3: {} seconds (cumulative)", timer.show());

  fmt::println("\n=== Random Functions Test Completed ===");
}

// needs more inputs to work...
void apportion_debug(int n, vector<float> splits) {
        fmt::println("\n=== APPORTION DEBUG (n={}) ===", n);

      vector<int> parts;
      for (size_t i = 0; i < splits.size(); ++i) {
        int part = static_cast<int>(round(n * splits[i]));
        parts.push_back(part);
        fmt::println("  parts[{}] = round({} * {}) = {}", i, n, splits[i], part);
      }

      // Calculate total before adjustment
      int total_before = std::accumulate(parts.begin(), parts.end(), 0);
      fmt::println("Total before adjustment: {}", total_before);

      // fix rounding error
      int diff = total_before - n;
      fmt::println("Difference (total - n): {}", diff);

      if (diff != 0) {
        fmt::println("Adjusting parts.back() from {} to {}", parts.back(), parts.back() - diff);
        parts.back() -= diff;
      }

      // Calculate total after adjustment
      int total_after = std::accumulate(parts.begin(), parts.end(), 0);
      fmt::println("Total after adjustment: {}", total_after);
      fmt::println("Expected (n): {}", n);
      fmt::println("Match: {}", total_after == n ? "YES" : "NO");
      fmt::println("=== END APPORTION DEBUG ===\n");
}

void sim_test(size_t ndays=180) {
  // fmt::print("\n=== Testing Spread Function (180-day simulation) ===\n\n");
  Model model = setup_sim(ndays, 38015, "2020-01-01", false);
  // fmt::println("Population: {}", model.pop.popn);
  // fmt::println("Running simulation for {} days...\n", model.ndays);

  runsim(model);

  // Count occurrences of infected and recovered --> gut check
    auto count = std::count_if(model.pop.variant_count.begin(), model.pop.variant_count.end(),
                               [](int x) { return x > 0; });
    fmt::println("Count of everyone who ever got infected:      {}", count);

    count = std::count_if(model.pop.variant_count.begin(), model.pop.variant_count.end(),
                               [](int x) { return x > 1; });
    fmt::println("Count of people who got sick 2 or more times: {}", count);

    count = std::count(model.pop.status.begin(), model.pop.status.end(), Trait::Stat::recovered);
    fmt::println("Count of everyone who recovered:              {}", count);

    count = std::count(model.pop.status.begin(), model.pop.status.end(), Trait::Stat::dead);
    fmt::println("Count of everyone who died:                   {}", count);


  fmt::println("\n=== Simulation Complete ===");
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

  // Test spread function n days simulation
  sim_test(180);

  return 0;
}