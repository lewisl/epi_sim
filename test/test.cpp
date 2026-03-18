
#include "../src/lib_includes.h"

#include "../src/parameters.h"
#include "../src/setup.h"
#include "../src/sim.h"
#include "../src/population.h"
#include "../src/population_print.h"
#include "../src/simple_print.h"
#include "../src/helpers.h"
#include "../src/random.h"
#include "../src/series.h"
#include "../src/timing.h"
#include "../src/spread.h"
#include "../src/progression.h"

#include <sstream>
#include <string_view>

using std::string;
using std::vector;
using std::string_view;

namespace poptable_test {

string rtrim_copy(string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

vector<string> split_trimmed_lines(const string& text) {
    std::istringstream input(text);
    vector<string> lines;
    string line;
    while (std::getline(input, line)) {
        lines.push_back(rtrim_copy(line));
    }
    return lines;
}

PopData make_popdata_print_fixture() {
    MapEnum<uint8_t> vax_lbl{{{"none", 0}, {"pfizer", 1}, {"moderna", 2}}};
    MapEnum<uint8_t> true_false{{{"false", 0}, {"true", 1}}};
    MapEnum<int> justint{};

    Variant::names = {"none", "alpha", "delta"};

    PopData pop(3, vax_lbl, true_false, justint);

    pop.agegrp[1] = Age::Age20_39;
    pop.agegrp[2] = Age::Age40_59;
    pop.agegrp[3] = Age::Age80_up;

    pop.status[1] = Stat::Recovered;
    pop.cond[1] = Cond::Uninfected;
    pop.variant[1][0] = Variant{1};
    pop.variant_count[1] = 1;
    pop.sickday[1][0] = 2;
    pop.recovday[1][0] = 9;
    pop.recovday_count[1] = 1;

    pop.status[2] = Stat::Infectious;
    pop.cond[2] = Cond::Mild;
    pop.duration[2] = 5;
    pop.ring[2] = 3;
    pop.quar[2] = 1;
    pop.quarday[2] = 8;
    pop.variant[2][0] = Variant{1};
    pop.variant[2][1] = Variant{2};
    pop.variant_count[2] = 2;
    pop.sickday[2][0] = 4;
    pop.sickday[2][1] = 11;
    pop.tested[2][0] = 0;
    pop.tested[2][1] = 1;
    pop.tested_count[2] = 2;
    pop.testday[2][0] = 6;
    pop.testday[2][1] = 12;
    pop.vaxstatus[2] = Vaxstat::booster;
    pop.vaxrcvd[2][0] = 1;
    pop.vaxrcvd[2][1] = 2;
    pop.vax_count[2] = 2;
    pop.vaxday[2][0] = 5;
    pop.vaxday[2][1] = 14;

    pop.status[3] = Stat::Unexposed;
    pop.cond[3] = Cond::Uninfected;

    return pop;
}

} // namespace poptable_test


void run_category_tests() {
    fmt::print("=== Testing New Namespace Approach ===\n\n");

    // Test Status namespace
    fmt::println("--- Testing Status Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Status::none = {} (expected 0)", static_cast<int>(Stat::None));
    fmt::println("  Status::unexposed = {} (expected 1)", static_cast<int>(Stat::Unexposed));
    fmt::println("  Status::infectious = {} (expected 2)", static_cast<int>(Stat::Infectious));
    fmt::println("  Status::recovered = {} (expected 3)", static_cast<int>(Stat::Recovered));
    fmt::println("  Status::dead = {} (expected 4)", static_cast<int>(Stat::Dead));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Status::to_str(0) = \"{}\" (expected \"none\")", Status::names[0]);
    fmt::println("  Status::to_str(1) = \"{}\" (expected \"unexposed\")", Status::names[1]);
    fmt::println("  Status::to_str(2) = \"{}\" (expected \"infectious\")", Status::names[2]);
    fmt::println("  Status::to_str(3) = \"{}\" (expected \"recovered\")", Status::names[3]);
    fmt::println("  Status::to_str(4) = \"{}\" (expected \"dead\")", Status::names[4]);
    fmt::println("  Status::from_str(\"unexposed\") = {} (expected 1)", static_cast<int>(Stat::Unexposed));
    fmt::println("  Status::from_str(\"infectious\") = {} (expected 2)", static_cast<int>(Stat::Infectious));
    fmt::println("  Status::from_str(\"recovered\") = {} (expected 3)", static_cast<int>(Stat::Recovered));
    fmt::println("  Status::from_str(\"dead\") = {} (expected 4)", static_cast<int>(Stat::Dead));
    fmt::println("  Status::from_str(\"invalid\") = {} (expected 99 - default to none)",
                 static_cast<int>(Stat::None));
    fmt::print("\n");

    // Test Condition namespace
    fmt::println("--- Testing Condition Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Condition::uninfected = {} (expected 0)", static_cast<int>(Cond::Uninfected));
    fmt::println("  Condition::nil = {} (expected 1)", static_cast<int>(Cond::Nil));
    fmt::println("  Condition::mild = {} (expected 2)", static_cast<int>(Cond::Mild));
    fmt::println("  Condition::sick = {} (expected 3)", static_cast<int>(Cond::Sick));
    fmt::println("  Condition::severe = {} (expected 4)", static_cast<int>(Cond::Severe));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Condition::to_str(0) = \"{}\" (expected \"uninfected\")", Condition::names[0]);
    fmt::println("  Condition::to_str(1) = \"{}\" (expected \"nil\")", Condition::names[1]);
    fmt::println("  Condition::to_str(2) = \"{}\" (expected \"mild\")", Condition::names[2]);
    fmt::println("  Condition::to_str(3) = \"{}\" (expected \"sick\")", Condition::names[3]);
    fmt::println("  Condition::to_str(4) = \"{}\" (expected \"severe\")", Condition::names[4]);
    fmt::println("  Condition::from_str(\"uninfected\") = {} (expected 0)", static_cast<int>(Cond::Uninfected));
    fmt::println("  Condition::from_str(\"nil\") = {} (expected 1)", static_cast<int>(Cond::Nil));
    fmt::println("  Condition::from_str(\"mild\") = {} (expected 2)", static_cast<int>(Cond::Mild));
    fmt::println("  Condition::from_str(\"sick\") = {} (expected 3)", static_cast<int>(Cond::Sick));
    fmt::println("  Condition::from_str(\"severe\") = {} (expected 4)", static_cast<int>(Cond::Severe));
    fmt::println("  Condition::from_str(\"invalid\") = {} (expected 99 - default to uninfected)",
                 static_cast<int>(Cond::Uninfected));
    fmt::println("{} expected \"uninfected\"", Condition::names[0]);

    fmt::print("\n");

    // Test Agegrp namespace
    fmt::println("--- Testing Agegrp Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Agegrp::unknown = {} (expected 0)", static_cast<int>(Age::Unknown));
    fmt::println("  Agegrp::age0_19 = {} (expected 1)", static_cast<int>(Age::Age0_19));
    fmt::println("  Agegrp::age20_39 = {} (expected 2)", static_cast<int>(Age::Age20_39));
    fmt::println("  Agegrp::age40_59 = {} (expected 3)", static_cast<int>(Age::Age40_59));
    fmt::println("  Agegrp::age60_79 = {} (expected 4)", static_cast<int>(Age::Age60_79));
    fmt::println("  Agegrp::age80_up = {} (expected 5)", static_cast<int>(Age::Age80_up));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Agegrp::to_str(0) = \"{}\" (expected \"unknown\")", Agegrp::names[0]);
    fmt::println("  Agegrp::to_str(1) = \"{}\" (expected \"age0_19\")", Agegrp::names[1]);
    fmt::println("  Agegrp::to_str(2) = \"{}\" (expected \"age20_39\")", Agegrp::names[2]);
    fmt::println("  Agegrp::to_str(3) = \"{}\" (expected \"age40_59\")", Agegrp::names[3]);
    fmt::println("  Agegrp::to_str(4) = \"{}\" (expected \"age60_79\")", Agegrp::names[4]);
    fmt::println("  Agegrp::to_str(5) = \"{}\" (expected \"age80_up\")", Agegrp::names[5]);
    fmt::println("  Agegrp::from_str(\"age0_19\") = {} (expected 1)", static_cast<int>(Age::Age0_19));
    fmt::println("  Agegrp::from_str(\"age20_39\") = {} (expected 2)", static_cast<int>(Age::Age20_39));
    fmt::println("  Agegrp::from_str(\"age40_59\") = {} (expected 3)", static_cast<int>(Age::Age40_59));
    fmt::println("  Agegrp::from_str(\"age60_79\") = {} (expected 4)", static_cast<int>(Age::Age60_79));
    fmt::println("  Agegrp::from_str(\"age80_up\") = {} (expected 5)", static_cast<int>(Age::Age80_up));
    fmt::println("  Agegrp::from_str(\"invalid\") = {} (expected 99 - default to unknown)", static_cast<int>(Age::Unknown));
    fmt::print("\n");

    // Test boundary conditions
    fmt::println("--- Testing Boundary Conditions ---");
    // fmt::println("  Status::to_str(99) = \"{}\" (expected \"INVALID\" - out of range)", Status::names[99]);
    // fmt::println("  Condition::to_str(99) = \"{}\" (expected \"INVALID\" - out of range)", Condition::names[99]);
    // fmt::println("  Agegrp::to_str(99) = \"{}\" (expected \"INVALID\" - out of range)", Agegrp::names[99]);
    // fmt::println("  Vaccine::to_str(99) = \"{}\" (expected \"unknown\" - out of range)", Vaccine::to_str(99));
    fmt::print("\n");

    fmt::println("=== All namespace tests completed ===");
}

void test_popdata_print_table() {
    fmt::print("\n=== Testing PopData Print Table ===\n\n");

    vector<string> saved_variant_names = Variant::names;
    PopData pop = poptable_test::make_popdata_print_fixture();
    vector<size_t> rows = {1, 2, 3};

    using PC = PopColumn;
    vector<PC> scalar_cols = {
        PC::status,
        PC::agegrp,
        PC::cond,
        PC::duration,
        PC::ring
    };

    std::ostringstream scalar_out;
    print_pop_table(pop, rows, scalar_cols, scalar_out);
    const auto scalar_lines = poptable_test::split_trimmed_lines(scalar_out.str());
    for (const auto& line : scalar_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");

    const vector<string> expected_scalar = {
        "row  status      agegrp    cond        duration  ring",
        "------------------------------------------------------",
        "  1  Recovered   Age20_39  Uninfected  0         0",
        "  2  Infectious  Age40_59  Mild        5         3",
        "  3  Unexposed   Age80_Up  Uninfected  0         0",
    };
    assert(scalar_lines == expected_scalar);

    vector<PC> mixed_cols = {
        PC::variant,
        PC::variant_count,
        PC::sickday,
        PC::recovday,
        PC::tested,
        PC::testday
    };

    std::ostringstream mixed_out;
    print_pop_table(pop, rows, mixed_cols, mixed_out);
    const auto mixed_lines = poptable_test::split_trimmed_lines(mixed_out.str());
    for (const auto& line : mixed_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");

    const vector<string> expected_mixed = {
        "row  variant  variant_count  sickday  recovday  tested  testday",
        "----------------------------------------------------------------",
        "  1  alpha    1              2        9         -       -",
        "  2  delta    2              11       -         true    12",
        "  3  none     0              -        -         -       -",
    };
    assert(mixed_lines == expected_mixed);

    vector<string_view> runtime_cols = {
        "vaxstatus",
        "vaxrcvd",
        "vax_count",
        "quar",
        "quarday"
    };

    std::ostringstream runtime_out;
    print_pop_table(pop, rows, runtime_cols, runtime_out);
    const auto runtime_lines = poptable_test::split_trimmed_lines(runtime_out.str());
    for (const auto& line : runtime_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");

    const vector<string> expected_runtime = {
        "row  vaxstatus  vaxrcvd  vax_count  quar   quarday",
        "---------------------------------------------------",
        "  1  none       -        0          false  0",
        "  2  booster    moderna  2          true   8",
        "  3  none       -        0          false  0",
    };
    assert(runtime_lines == expected_runtime);

    std::ostringstream multi_out;
    print_pop_table(pop, rows, mixed_cols, multi_out, true);
    const auto multi_lines = poptable_test::split_trimmed_lines(multi_out.str());
    for (const auto& line : multi_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");

    const vector<string> expected_multi = {
        "row  variant  variant_count  sickday  recovday  tested  testday",
        "----------------------------------------------------------------",
        "  1  alpha    1              2        9         -       -",
        "  2  alpha    2              4                  false   6",
        "  *  delta                   11                 true    12",
        "  3  none     0              -        -         -       -",
    };
    assert(multi_lines == expected_multi);

    bool bad_row_threw = false;
    try {
        const vector<size_t> bad_rows = {0, 1};
        print_pop_table(pop, bad_rows, scalar_cols);
    } catch (const std::invalid_argument&) {
        bad_row_threw = true;
    }
    assert(bad_row_threw);

    bool bad_column_threw = false;
    try {
        const vector<string_view> bad_cols = {"status", "does_not_exist"};
        print_pop_table(pop, rows, bad_cols);
    } catch (const std::invalid_argument&) {
        bad_column_threw = true;
    }
    assert(bad_column_threw);

    Variant::names = saved_variant_names;

    fmt::println("=== PopData Print Table Test Completed ===");
}

void test_simple_pop_print() {
    fmt::print("\n=== Testing Simple Pop Printer ===\n\n");

    vector<string> saved_variant_names = Variant::names;
    PopData pop = poptable_test::make_popdata_print_fixture();
    vector<size_t> rows = {1, 2, 3};

    vector<string_view> scalar_cols = {
        "status",
        "agegrp",
        "cond",
        "duration",
        "ring"
    };

    std::ostringstream scalar_out;
    print_simple_pop(pop, rows, scalar_cols, scalar_out);
    const auto scalar_lines = poptable_test::split_trimmed_lines(scalar_out.str());
    for (const auto& line : scalar_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");
    const vector<string> expected_scalar = {
        "row  status      agegrp    cond        duration  ring",
        "------------------------------------------------------",
        "  1  Recovered   Age20_39  Uninfected  0         0",
        "  2  Infectious  Age40_59  Mild        5         3",
        "  3  Unexposed   Age80_Up  Uninfected  0         0",
    };
    assert(scalar_lines == expected_scalar);

    vector<string_view> mixed_cols = {
        "variant",
        "variant_count",
        "sickday",
        "recovday",
        "tested",
        "testday"
    };

    std::ostringstream mixed_out;
    print_simple_pop(pop, rows, mixed_cols, mixed_out);
    const auto mixed_lines = poptable_test::split_trimmed_lines(mixed_out.str());
    for (const auto& line : mixed_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");
    const vector<string> expected_mixed = {
        "row  variant  variant_count  sickday  recovday  tested  testday",
        "----------------------------------------------------------------",
        "  1  alpha    1              2        9         -       -",
        "  2  delta    2              11       -         true    12",
        "  3  none     0              -        -         -       -",
    };
    assert(mixed_lines == expected_mixed);

    vector<string_view> runtime_cols = {
        "vaxstatus",
        "vaxrcvd",
        "vax_count",
        "quar",
        "quarday"
    };

    std::ostringstream runtime_out;
    print_simple_pop(pop, rows, runtime_cols, runtime_out);
    const auto runtime_lines = poptable_test::split_trimmed_lines(runtime_out.str());
    for (const auto& line : runtime_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");
    const vector<string> expected_runtime = {
        "row  vaxstatus  vaxrcvd  vax_count  quar   quarday",
        "---------------------------------------------------",
        "  1  none       -        0          false  0",
        "  2  booster    moderna  2          true   8",
        "  3  none       -        0          false  0",
    };
    assert(runtime_lines == expected_runtime);

    bool bad_row_threw = false;
    try {
        const vector<size_t> bad_rows = {0, 1};
        print_simple_pop(pop, bad_rows, scalar_cols);
    } catch (const std::invalid_argument&) {
        bad_row_threw = true;
    }
    assert(bad_row_threw);

    bool big_row_threw = false;
    try {
        const vector<size_t> bad_rows = {1, 4};
        print_simple_pop(pop, bad_rows, scalar_cols);
    } catch (const std::invalid_argument&) {
        big_row_threw = true;
    }
    assert(big_row_threw);

    bool bad_column_threw = false;
    try {
        const vector<string_view> bad_cols = {"status", "does_not_exist"};
        print_simple_pop(pop, rows, bad_cols);
    } catch (const std::invalid_argument&) {
        bad_column_threw = true;
    }
    assert(bad_column_threw);

    Variant::names = saved_variant_names;

    fmt::println("=== Simple Pop Printer Test Completed ===");
}

void test_finalize_series() {
    fmt::print("\n=== Testing Finalize Series ===\n\n");

    DayData series(4);
    auto& now_infected_total = series.at(SeriesName::now_infected, AgeBucket::total);
    auto& net_infected_total = series.at(SeriesName::net_infected, AgeBucket::total);
    auto& now_infected_40_59 = series.at(SeriesName::now_infected, AgeBucket::age40_59);
    auto& net_infected_40_59 = series.at(SeriesName::net_infected, AgeBucket::age40_59);

    now_infected_total[0] = 99;
    net_infected_total[0] = 77;
    now_infected_total[1] = 3;
    now_infected_total[2] = 5;
    now_infected_total[3] = 5;
    now_infected_total[4] = 9;

    now_infected_40_59[0] = 42;
    net_infected_40_59[0] = 24;
    now_infected_40_59[1] = 1;
    now_infected_40_59[2] = 1;
    now_infected_40_59[3] = 4;
    now_infected_40_59[4] = 4;

    finalize_series(series);

    const vector<size_t> expected_total = {77, 3, 2, 0, 4};
    const vector<size_t> expected_age = {24, 1, 0, 3, 0};

    assert(net_infected_total == expected_total);
    assert(net_infected_40_59 == expected_age);
    assert(now_infected_total[0] == 99);
    assert(now_infected_40_59[0] == 42);

    DayData single_day_series(1);
    auto& single_now_infected_total = single_day_series.at(SeriesName::now_infected, AgeBucket::total);
    auto& single_net_infected_total = single_day_series.at(SeriesName::net_infected, AgeBucket::total);
    single_now_infected_total[0] = 11;
    single_net_infected_total[0] = 22;
    single_now_infected_total[1] = 6;

    finalize_series(single_day_series);

    assert(single_net_infected_total[0] == 22);
    assert(single_net_infected_total[1] == 6);

    fmt::println("=== Finalize Series Test Completed ===");
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
                     Agegrp::names[i], age_counts[i], actual_pct, expected_pct, diff);
    }

    fmt::println("\nNote: Small differences are expected due to rounding when distributing");
    fmt::println("      {} people across age groups.", pop.popn);

    // Verify total
    int total = std::accumulate(age_counts.begin(), age_counts.end(), 0);
    fmt::println("\nTotal count verification: {} (should equal {})", total, pop.popn);

    fmt::println("\n=== Age Distribution Test Completed ===");
}

void test_model_params(ModelParams mp, Model model) {
  fmt::println("==================== Model Parameters ==================");
  mp.geodata.print();
  fmt::print("\n");
  print_trait_vector(mp.variants);
  fmt::print("\n");
  print_infectparams(mp.infectparams, mp.variants);
  fmt::print("\n");
  mp.socialdata.print();
  fmt::print("\n");
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

  fmt::println("==================== Simulation Parameters ==================");
  fmt::println(" first day of sim: {} -- last day of sim: {}\n\n",
              absl::FormatCivilTime(model.caldays.front()),
              absl::FormatCivilTime(model.caldays.back()));
  fmt::println("==================== indoor_seq ==================");
  int item_ctr{0};
  for (auto factor : model.indoor_seq) {
    fmt::print("{:5}", factor);
    ++item_ctr;
    if (item_ctr % 15 == 0) fmt::println("\n");
  }
  fmt::println("\n");

}

void test_popdata_size(Model model) {
  fmt::println("Vector sizing: {} popz {}", model.pop.status.size(), model.pop.popz);
  fmt::println("Index to actual population size: {}",
               Agegrp::names[model.pop.agegrp[model.pop.popn]]);
}

void test_multiple_infections() {
  fmt::print("\n=== Testing Multiple Infections Over Time ===\n\n");

  // Create a fresh model for this test
  fmt::println("Setting up independent test environment...");
  Model test_model = setup_sim(1000, 38015, "2020-01-01", false);
  PopData& pop = test_model.pop;
  vector<Variant>& variants = test_model.mp.variants;

  // Find one person in age20_39 and one in age60_79
  size_t idx_age20_39 = 0;
  size_t idx_age60_79 = 0;

  for (size_t i = 1; i <= pop.popn; ++i) {
    if (idx_age20_39 == 0 && pop.agegrp[i] == Age::Age20_39) {
      idx_age20_39 = i;
    }
    if (idx_age60_79 == 0 && pop.agegrp[i] == Age::Age60_79) {
      idx_age60_79 = i;
    }
    if (idx_age20_39 != 0 && idx_age60_79 != 0) {
      break;
    }
  }

  if (idx_age20_39 == 0 || idx_age60_79 == 0) {
    fmt::println("ERROR: Could not find required age groups in population");
    return;
  }

  // resolve idx for person to the person's row of traits:
  auto person_age20_39 = pop.agent(idx_age20_39);
  auto person_age60_79 = pop.agent(idx_age60_79);

  fmt::println("Testing with:");
  fmt::println("  Person {} (age group: {}) - will be infected 10 times",
               person_age20_39.id, Agegrp::names[person_age20_39.agegrp()]);
  fmt::println("  Person {} (age group: {}) - will be infected 17 times\n",
               person_age60_79.id, Agegrp::names[person_age60_79.agegrp()]);

  // Use first real variant (index 1, since 0 is "none")
  Variant base_variant = test_model.mp.variants[1];
  Condition condition = Cond::Nil;
  uint8_t duration = 5;

  // Reset sim day to 1
  sim::current_day = 1;
  DayData series(400);

  // Infect person 1 ten times (days 1, 21, 41, 61, 81, 101, 121, 141, 161, 181)
  fmt::println("Infecting person {} 10 times:", person_age20_39.id);
  for (int infection = 0; infection < 10; ++infection) {
    int day = 1 + (infection * 20);
    sim::current_day = day;
    pop.make_sick(person_age20_39, base_variant, series, condition, duration);
    fmt::println("  Infection {} on day {} - variant_count: {}",
                 infection + 1, day, static_cast<int>(person_age20_39.variant_count()));
  }

  fmt::println("\nPerson {} final state:", person_age20_39.id);
  fmt::println("  variant_count: {}", static_cast<int>(person_age20_39.variant_count()));
  fmt::println("  Infection history (variant, sickday):");
  int count1 = std::min(static_cast<int>(person_age20_39.variant_count()), 16);
  for (int i = 0; i < count1; ++i) {
    fmt::println("    [{}] variant: {}, sickday: {}",
                 i, person_age20_39.get_variant().name(), person_age20_39.get_sickday());
  }

  // Infect person 2 seventeen times (days 1, 21, 41, ..., 321)
  fmt::print("\n\n");
  fmt::println("Infecting person {} 17 times:", person_age60_79.id);
  for (int infection = 0; infection < 17; ++infection) {
    int day = 1 + (infection * 20);
    sim::current_day = day;
    pop.make_sick(person_age60_79, base_variant, series, condition, duration);
    fmt::println("  Infection {} on day {} - variant_count: {}",
                 infection + 1, day, static_cast<int>(person_age60_79.variant_count()));
  }

  fmt::println("\nPerson {} final state:", person_age60_79.id);
  fmt::println("  variant_count: {}", static_cast<int>(person_age60_79.variant_count()));
  fmt::println("  Infection history (variant, sickday):");
  int count2 = std::min(static_cast<int>(person_age60_79.variant_count()), 16);
  fmt::println("  (Showing {} most recent infections, max capacity is 16)", count2);
  for (int i = 0; i < count2; ++i) {
    fmt::println("    [{}] variant: {}, sickday: {}",
                 i, person_age60_79.get_variant().name(), person_age60_79.get_sickday());
  }

  fmt::println("\n=== Multiple Infections Test Completed ===");
}

void test_seedcase_multiple_infections() {
  fmt::print("\n=== Testing SeedCase with Multiple Infections ===\n\n");

  // Create a fresh model for this test
  fmt::println("Setting up independent test environment...");
  Model test_model = setup_sim(1000, 38015, "2020-01-01", false);
  PopData& pop = test_model.pop;
  DayData series(321);

  // Use first real variant (index 1, since 0 is "none")
  Variant base_variant = test_model.mp.variants[1];
  Condition condition = Cond::Nil;
  uint8_t duration = 5;

  // Create SeedCases:
  // - One for age20_39 that triggers 10 times (days 1, 21, 41, ..., 181)
  // - One for age60_79 that triggers 17 times (days 1, 21, 41, ..., 321)

  std::vector<SeedCase> seed_cases;

  // Create 10 seed cases for age20_39 (one person each time)
  for (int i = 0; i < 10; ++i) {
    int trigger_day = 1 + (i * 20);
    std::vector<SeedFilter> filters{{Age::Age20_39, condition, 0, base_variant, 1}};
    seed_cases.emplace_back(trigger_day, true, filters, pop);
  }

  // Create 17 seed cases for age60_79 (one person each time)
  for (int i = 0; i < 17; ++i) {
    int trigger_day = 1 + (i * 20);
    std::vector<SeedFilter> filters{{Age::Age60_79, condition, 0, base_variant, 1}};
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
        auto seeded = seed_case(series);

        // Track which persons were seeded
        if (!seeded.empty()) {
          size_t person = seeded[0];
          if (pop.agegrp[person] == Age::Age20_39) {
            person_age20_39 = person;
            fmt::println("  Day {}: Infected person {} (age20_39), variant_count now: {}",
                         day, person, static_cast<int>(pop.variant_count[person]));
          } else if (pop.agegrp[person] == Age::Age60_79) {
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
                 i, pop.variant[person_age20_39][i].name(), pop.sickday[person_age20_39][i]);
  }

  fmt::println("\nPerson {} (age60_79) final state:", person_age60_79);
  fmt::println("  variant_count: {}", static_cast<int>(pop.variant_count[person_age60_79]));
  fmt::println("  Infection history (variant, sickday):");
  int count2 = std::min(static_cast<int>(pop.variant_count[person_age60_79]), 16);
  fmt::println("  (Showing {} most recent infections, max capacity is 16)", count2);
  for (int i = 0; i < count2; ++i) {
    fmt::println("    [{}] variant: {}, sickday: {}",
                 i, pop.variant[person_age60_79][i].name(), pop.sickday[person_age60_79][i]);
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

void sim_test(size_t ndays=180, int locale=38015) {
  // fmt::print("\n=== Testing Spread Function (180-day simulation) ===\n\n");
  Model model = setup_sim(ndays, locale, "2020-01-01", false);
  // fmt::println("Population: {}", model.pop.popn);
  // fmt::println("Running simulation for {} days...\n", model.ndays);

  runsim(model);
 
}

int main() {
  // Test seeding and spread contact generation
  // test_seeding_and_spread();

  // Unit test: PopData table printer
  // test_popdata_print_table();
  // test_simple_pop_print();
  test_finalize_series();

  // Test random number generator functions
  // test_random_functions();

  // tests
  // run_category_tests();

  // Test model params
  // Model model = setup_sim(180, 38015, "2020-02-01", true);
  // test_model_params(model.mp, model);

  // test age distribution
  // test_age_distribution(setup_sim(1000, 38015, "2020-01-01", false).pop);

  // Test full simulation
  sim_test(180, 38015);

}
