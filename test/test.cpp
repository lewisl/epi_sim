
#include "../src/lib_includes.h"

#include "../src/parameters.h"
#include "../src/setup.h"
#include "../src/sim.h"
#include "../src/population.h"
#include "../src/agent_pop_print.h"
#include "../src/helpers.h"
#include "../src/random.h"
#include "../src/series.h"
#include "../src/timing.h"
#include "../src/spread.h"
#include "../src/progression.h"
#include "../src/disease_modeling.h"
#include "../src/vaccination.h"
#include "../src/plot.h"

#include <sstream>
#include <string_view>

using std::string;
using std::vector;
using std::string_view;

namespace parameter_test {

namespace fs = std::filesystem;

struct VariantNamesGuard {
    vector<string> saved_names = Variant::names;

    ~VariantNamesGuard() {
        Variant::names = saved_names;
    }
};

struct VaxNamesGuard {
    vector<string> saved_names = Vax::names;

    ~VaxNamesGuard() {
        Vax::names = saved_names;
    }
};

struct SampleParamPaths {
    string geodata;
    string variants;
    string social;
    string vaccines;
    string vax_sched_dir;
};

fs::path project_dir() {
    const fs::path cwd = fs::current_path();
    if (fs::exists(cwd / "sample_parameters")) {
        return cwd;
    }
    return fs::path(std::getenv("HOME")) / "code" / "epi_sim";
}

SampleParamPaths sample_paths() {
    const fs::path root = project_dir();
    return {
        .geodata = (root / "sample_parameters" / "geo2data.csv").string(),
        .variants = (root / "sample_parameters" / "variants.json").string(),
        .social = (root / "sample_parameters" / "socialparams.json").string(),
        .vaccines = (root / "sample_parameters" / "vaccines.json").string(),
        .vax_sched_dir = (root / "sample_parameters" / "vaccine_100k").string(),
    };
}

size_t require_locale_index(const GeoData& geodata, int locale) {
    const auto it = std::find(geodata.fips.begin(), geodata.fips.end(), locale);
    assert(it != geodata.fips.end());
    return static_cast<size_t>(std::distance(geodata.fips.begin(), it));
}

const VaxParams& require_vax(const VaxSet& vaxset, string_view name) {
    const auto it = std::find(Vax::names.begin(), Vax::names.end(), name);
    assert(it != Vax::names.end());
    return vaxset.at(Vax{static_cast<uint8_t>(std::distance(Vax::names.begin(), it))});
}

const PerVaxSpec& require_sched_vax(const VaxSched& sched, string_view name) {
    const auto it = std::find_if(sched.vaxesincluded.begin(), sched.vaxesincluded.end(),
                                 [name](const auto& entry) { return entry.vax.show() == name; });
    assert(it != sched.vaxesincluded.end());
    return *it;
}

const VaxSched& require_sched(const VaxSchedSet& schedset, string_view name) {
    const auto it = std::find_if(schedset.schedules.begin(), schedset.schedules.end(),
                                 [name](const auto& entry) { return entry.first == name; });
    assert(it != schedset.schedules.end());
    return it->second;
}

float require_named_factor(const vector<std::pair<string, float>>& entries, string_view name) {
    const auto it = std::find_if(entries.begin(), entries.end(),
                                 [name](const auto& entry) { return entry.first == name; });
    assert(it != entries.end());
    return it->second;
}

size_t require_calday_index(const vector<absl::CivilDay>& caldays, const absl::CivilDay& day) {
    const auto it = std::find(caldays.begin(), caldays.end(), day);
    assert(it != caldays.end());
    return static_cast<size_t>(std::distance(caldays.begin(), it));
}

} // namespace parameter_test

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
    Variant::names = {"none", "alpha", "delta"};
    Vax::names = {"none", "pfizer", "moderna"};

    PopData pop(3);

    pop.agegrp[1] = AGE20_39;
    pop.agegrp[2] = AGE40_59;
    pop.agegrp[3] = AGE80_UP;

    pop.status[1] = RECOVERED;
    pop.cond[1] = UNINFECTED;
    pop.variant[1] = Variant{1};
    pop.variant_hist[1].arr[0] = Variant{1};
    pop.variant_hist[1].count = 1;
    pop.sickday[1] = 2;
    pop.sickday_hist[1].arr[0] = 2;
    pop.sickday_hist[1].count = 1;
    pop.recovday[1] = 9;
    pop.recovday_hist[1].arr[0] = 9;
    pop.recovday_hist[1].count = 1;

    pop.status[2] = INFECTIOUS;
    pop.cond[2] = MILD;
    pop.duration[2] = 5;
    pop.ring[2] = 3;
    pop.quar[2] = 1;
    pop.quarday[2] = 8;
    pop.variant[2] = Variant{2};
    pop.variant_hist[2].arr[0] = Variant{1};
    pop.variant_hist[2].arr[1] = Variant{2};
    pop.variant_hist[2].count = 2;
    pop.sickday[2] = 11;
    pop.sickday_hist[2].arr[0] = 4;
    pop.sickday_hist[2].arr[1] = 11;
    pop.sickday_hist[2].count = 2;
    pop.testday[2] = 12;
    pop.testday_hist[2].arr[0] = 6;
    pop.testday_hist[2].arr[1] = 12;
    pop.testday_hist[2].count = 2;
    pop.vaxstatus[2] = Vaxstat::booster;
    pop.vax[2] = Vax{2};
    pop.vax_hist[2].arr[0] = Vax{1};
    pop.vax_hist[2].arr[1] = Vax{2};
    pop.vax_hist[2].count = 2;
    pop.vaxday[2] = 14;
    pop.vaxday_hist[2].arr[0] = 5;
    pop.vaxday_hist[2].arr[1] = 14;
    pop.vaxday_hist[2].count = 2;

    pop.status[3] = UNEXPOSED;
    pop.cond[3] = UNINFECTED;

    return pop;
}

} // namespace poptable_test

void test_primitive_column_wrappers() {
    fmt::print("\n=== Testing Primitive Column Wrappers ===\n\n");

    Duration duration;
    Sickday sickday;

    assert(duration == 0);
    assert(sickday == 0);

    duration = 5;
    sickday = 12;

    assert(duration.show() == "5");
    assert(sickday.show() == "12");
    assert(duration.show() == "5");
    assert(sickday.show() == "12");

    assert(duration == 5);
    assert(5 == duration);
    assert(duration < 6);
    assert(4 < duration);
    assert(sickday == 12);
    assert(sickday > 11);

    Duration next = duration + 2;
    assert(next == 7);
    next += 3;
    assert(next == 10);
    const Duration before_post = next++;
    assert(before_post == 10);
    assert(next == 11);
    ++next;
    assert(next == 12);

    fmt::println("=== Primitive Column Wrappers Test Completed ===");
}


void run_category_tests() {
    fmt::print("=== Testing New Namespace Approach ===\n\n");

    // Test Status constants
    fmt::println("--- Testing Status Constants ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Status::none = {} (expected 0)", static_cast<int>(NONE));
    fmt::println("  Status::unexposed = {} (expected 1)", static_cast<int>(UNEXPOSED));
    fmt::println("  Status::infectious = {} (expected 2)", static_cast<int>(INFECTIOUS));
    fmt::println("  Status::recovered = {} (expected 3)", static_cast<int>(RECOVERED));
    fmt::println("  Status::dead = {} (expected 4)", static_cast<int>(DEAD));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Status::to_str(0) = \"{}\" (expected \"none\")", Status::names[0]);
    fmt::println("  Status::to_str(1) = \"{}\" (expected \"unexposed\")", Status::names[1]);
    fmt::println("  Status::to_str(2) = \"{}\" (expected \"infectious\")", Status::names[2]);
    fmt::println("  Status::to_str(3) = \"{}\" (expected \"recovered\")", Status::names[3]);
    fmt::println("  Status::to_str(4) = \"{}\" (expected \"dead\")", Status::names[4]);
    fmt::println("  Status::from_str(\"unexposed\") = {} (expected 1)", static_cast<int>(UNEXPOSED));
    fmt::println("  Status::from_str(\"infectious\") = {} (expected 2)", static_cast<int>(INFECTIOUS));
    fmt::println("  Status::from_str(\"recovered\") = {} (expected 3)", static_cast<int>(RECOVERED));
    fmt::println("  Status::from_str(\"dead\") = {} (expected 4)", static_cast<int>(DEAD));
    fmt::println("  Status::from_str(\"invalid\") = {} (expected 99 - default to none)",
                 static_cast<int>(NONE));
    fmt::print("\n");

    // Test Condition namespace
    fmt::println("--- Testing Condition Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Condition::uninfected = {} (expected 0)", static_cast<int>(UNINFECTED));
    fmt::println("  Condition::nil = {} (expected 1)", static_cast<int>(NIL));
    fmt::println("  Condition::mild = {} (expected 2)", static_cast<int>(MILD));
    fmt::println("  Condition::sick = {} (expected 3)", static_cast<int>(SICK));
    fmt::println("  Condition::severe = {} (expected 4)", static_cast<int>(SEVERE));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Condition::to_str(0) = \"{}\" (expected \"uninfected\")", Condition::names[0]);
    fmt::println("  Condition::to_str(1) = \"{}\" (expected \"nil\")", Condition::names[1]);
    fmt::println("  Condition::to_str(2) = \"{}\" (expected \"mild\")", Condition::names[2]);
    fmt::println("  Condition::to_str(3) = \"{}\" (expected \"sick\")", Condition::names[3]);
    fmt::println("  Condition::to_str(4) = \"{}\" (expected \"severe\")", Condition::names[4]);
    fmt::println("  Condition::from_str(\"uninfected\") = {} (expected 0)", static_cast<int>(UNINFECTED));
    fmt::println("  Condition::from_str(\"nil\") = {} (expected 1)", static_cast<int>(NIL));
    fmt::println("  Condition::from_str(\"mild\") = {} (expected 2)", static_cast<int>(MILD));
    fmt::println("  Condition::from_str(\"sick\") = {} (expected 3)", static_cast<int>(SICK));
    fmt::println("  Condition::from_str(\"severe\") = {} (expected 4)", static_cast<int>(SEVERE));
    fmt::println("  Condition::from_str(\"invalid\") = {} (expected 99 - default to uninfected)",
                 static_cast<int>(UNINFECTED));
    fmt::println("{} expected \"uninfected\"", Condition::names[0]);

    fmt::print("\n");

    // Test Agegrp namespace
    fmt::println("--- Testing Agegrp Namespace ---");
    fmt::println("Enum values (0-indexed):");
    fmt::println("  Agegrp::unknown = {} (expected 0)", static_cast<int>(UNKNOWN));
    fmt::println("  Agegrp::age0_19 = {} (expected 1)", static_cast<int>(AGE0_19));
    fmt::println("  Agegrp::age20_39 = {} (expected 2)", static_cast<int>(AGE20_39));
    fmt::println("  Agegrp::age40_59 = {} (expected 3)", static_cast<int>(AGE40_59));
    fmt::println("  Agegrp::age60_79 = {} (expected 4)", static_cast<int>(AGE60_79));
    fmt::println("  Agegrp::age80_up = {} (expected 5)", static_cast<int>(AGE80_UP));
    fmt::print("\nRound-trip tests:\n");
    fmt::println("  Agegrp::to_str(0) = \"{}\" (expected \"unknown\")", Agegrp::names[0]);
    fmt::println("  Agegrp::to_str(1) = \"{}\" (expected \"age0_19\")", Agegrp::names[1]);
    fmt::println("  Agegrp::to_str(2) = \"{}\" (expected \"age20_39\")", Agegrp::names[2]);
    fmt::println("  Agegrp::to_str(3) = \"{}\" (expected \"age40_59\")", Agegrp::names[3]);
    fmt::println("  Agegrp::to_str(4) = \"{}\" (expected \"age60_79\")", Agegrp::names[4]);
    fmt::println("  Agegrp::to_str(5) = \"{}\" (expected \"age80_up\")", Agegrp::names[5]);
    fmt::println("  Agegrp::from_str(\"age0_19\") = {} (expected 1)", static_cast<int>(AGE0_19));
    fmt::println("  Agegrp::from_str(\"age20_39\") = {} (expected 2)", static_cast<int>(AGE20_39));
    fmt::println("  Agegrp::from_str(\"age40_59\") = {} (expected 3)", static_cast<int>(AGE40_59));
    fmt::println("  Agegrp::from_str(\"age60_79\") = {} (expected 4)", static_cast<int>(AGE60_79));
    fmt::println("  Agegrp::from_str(\"age80_up\") = {} (expected 5)", static_cast<int>(AGE80_UP));
    fmt::println("  Agegrp::from_str(\"invalid\") = {} (expected 99 - default to unknown)", static_cast<int>(UNKNOWN));
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





void test_agent_pop_print() {
    fmt::print("\n=== Testing Agent Pop Printer ===\n\n");

    vector<string> saved_variant_names = Variant::names;
    PopData pop = poptable_test::make_popdata_print_fixture();
    vector<size_t> rows = {1, 2, 3};

    std::ostringstream scalar_out;
    print_agent_pop_table(pop, rows,
                          {"status", "agegrp", "cond", "duration", "ring"},
                          scalar_out);
    const auto scalar_lines = poptable_test::split_trimmed_lines(scalar_out.str());
    for (const auto& line : scalar_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");
    const vector<string> expected_scalar = {
        "row  status      agegrp      cond        duration  ring",
        "-------------------------------------------------------",
        "  1  recovered   age20_39    uninfected  0         0",
        "  2  infectious  age40_59    mild        5         3",
        "  3  unexposed   age80_up    uninfected  0         0",
    };
    assert(scalar_lines == expected_scalar);

    std::ostringstream mixed_out;
    print_agent_pop_table(pop, rows,
                          {"variant", "variant_hist", "sickday", "sickday_hist",
                           "recovday", "recovday_hist", "testday", "testday_hist"},
                          mixed_out);
    const auto mixed_lines = poptable_test::split_trimmed_lines(mixed_out.str());
    for (const auto& line : mixed_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");
    const vector<string> expected_mixed = {
        "row  variant     variant_hist   sickday  sickday_hist  recovday  recovday_hist  testday  testday_hist",
        "--------------------------------------------------------------------------------------------------------",
        "  1  alpha       alpha         2        2             9         9              -        -",
        "  2  delta       alpha|delta   11       4|11          -         -              12       6|12",
        "  3  none        -             -        -             -         -              -        -",
    };
    assert(mixed_lines == expected_mixed);

    std::ostringstream multi_out;
    print_agent_pop_table(pop, rows,
                          {"variant", "variant_hist", "sickday", "sickday_hist",
                           "recovday", "recovday_hist", "testday", "testday_hist"},
                          multi_out, true);
    const auto multi_lines = poptable_test::split_trimmed_lines(multi_out.str());
    for (const auto& line : multi_lines) {
        fmt::println("{}", line);
    }
    fmt::print("\n");
    const vector<string> expected_multi = {
        "row  variant     variant_hist   sickday  sickday_hist  recovday  recovday_hist  testday  testday_hist",
        "--------------------------------------------------------------------------------------------------------",
        "  1  alpha       alpha         2        2             9         9              -        -",
        "  2  delta       alpha         11       4             -         -              12       6",
        "  *             delta                  11                                      12",
        "  3  none        -             -        -             -         -              -        -",
    };
    assert(multi_lines == expected_multi);

    std::ostringstream init_list_out;
    print_agent_pop_table(pop, rows, {"status", "variant", "sickday"}, init_list_out);
    std::ostringstream init_list_expected_out;
    print_agent_pop_table(pop, rows, {"status", "variant", "sickday"}, init_list_expected_out);
    assert(poptable_test::split_trimmed_lines(init_list_out.str()) ==
           poptable_test::split_trimmed_lines(init_list_expected_out.str()));

    Variant::names = {"none", "alpha_variant_extra", "delta"};
    std::ostringstream truncation_out;
    const vector<size_t> truncation_rows = {1};
    print_agent_pop_table(pop, truncation_rows, {"variant"}, truncation_out);
    const vector<string> expected_truncation = {
        "row  variant",
        "---------------",
        "  1  alpha_vari",
    };
    assert(poptable_test::split_trimmed_lines(truncation_out.str()) == expected_truncation);
    Variant::names = saved_variant_names;

    const string all_cols_runtime = "all";
    std::ostringstream all_out;
    std::ostringstream all_expected_out;
    print_agent_pop_table(pop, rows, all_cols_runtime, all_out);
    print_agent_pop_table(
        pop, rows,
        {"status", "agegrp", "cond", "duration", "variant", "variant_hist",
         "sickday", "sickday_hist",
         "recovday", "recovday_hist", "deadday", "ring", "sdcase",
         "testday_hist", "testday", "quar", "quarday", "vaxstatus",
         "vax", "vax_hist", "vaxday", "vaxday_hist"},
        all_expected_out);
    assert(poptable_test::split_trimmed_lines(all_out.str()) ==
           poptable_test::split_trimmed_lines(all_expected_out.str()));

    bool bad_zero_row_threw = false;
    try {
        const vector<size_t> bad_rows = {0, 1};
        print_agent_pop_table(pop, bad_rows,
                              {"status", "agegrp", "cond", "duration", "ring"});
    } catch (const std::invalid_argument&) {
        bad_zero_row_threw = true;
    }
    assert(bad_zero_row_threw);

    bool bad_big_row_threw = false;
    try {
        const vector<size_t> bad_rows = {1, 4};
        print_agent_pop_table(pop, bad_rows,
                              {"status", "agegrp", "cond", "duration", "ring"});
    } catch (const std::invalid_argument&) {
        bad_big_row_threw = true;
    }
    assert(bad_big_row_threw);

    bool bad_column_threw = false;
    try {
        print_agent_pop_table(pop, rows, {"status", "does_not_exist"});
    } catch (const std::invalid_argument&) {
        bad_column_threw = true;
    }
    assert(bad_column_threw);

    bool bad_runtime_selector_threw = false;
    try {
        print_agent_pop_table(pop, rows, string{"status"});
    } catch (const std::invalid_argument&) {
        bad_runtime_selector_threw = true;
    }
    assert(bad_runtime_selector_threw);

    Variant::names = saved_variant_names;

    fmt::println("=== Agent Pop Printer Test Completed ===");
}

void test_pop_column_registry() {
    fmt::print("\n=== Testing Pop Column Registry ===\n\n");

    PopData pop = poptable_test::make_popdata_print_fixture();

    assert(PopData::column_map().size() == size_t(ColumnName::COUNT));
    for (const auto& [key, spec] : PopData::column_map()) {
        assert(key == to_string(spec.name));
        assert(PopData::find_column(spec.name) != nullptr);
        assert(PopData::find_column(to_string(spec.name))->name == spec.name);
        assert(PopData::find_column(key) != nullptr);
        assert(PopData::find_column(key)->name == spec.name);
    }

    assert(PopData::column_name_from_string("status") == ColumnName::status);
    assert(PopData::column_name_from_string("sickday_hist") == ColumnName::sickday_hist);
    assert(PopData::column_name_from_string("recovday_hist") == ColumnName::recovday_hist);
    assert(PopData::column_name_from_string("testday_hist") == ColumnName::testday_hist);
    assert(PopData::column_name_from_string("testday") == ColumnName::testday);
    assert(PopData::column_name_from_string("vax") == ColumnName::vax);
    assert(PopData::column_name_from_string("vax_hist") == ColumnName::vax_hist);
    assert(PopData::column_name_from_string("vaxday_hist") == ColumnName::vaxday_hist);
    assert(!PopData::column_name_from_string("variant_count").has_value());
    assert(!PopData::column_name_from_string("does_not_exist").has_value());

    const auto row1 = pop.agent(1);
    const auto row2 = pop.agent(2);
    const auto row3 = pop.agent(3);

    assert(PopData::find_column("status")->to_txt_cell(row2) == "infectious");
    assert(PopData::find_column("variant")->to_txt_cell(row2) == "delta");
    assert(PopData::find_column("vax")->to_txt_cell(row2) == "moderna");

    assert(PopData::find_column("status")->to_txt_cell(row2) == "infectious");
    assert(PopData::find_column("agegrp")->to_txt_cell(row2) == "age40_59");
    assert(PopData::find_column("cond")->to_txt_cell(row2) == "mild");
    assert(PopData::find_column("duration")->to_txt_cell(row2) == "5");
    assert(PopData::find_column("ring")->to_txt_cell(row2) == "3");
    assert(PopData::find_column("sdcase")->to_txt_cell(row2) == "false");
    assert(PopData::find_column("quar")->to_txt_cell(row2) == "true");
    assert(PopData::find_column("quarday")->to_txt_cell(row2) == "8");
    assert(PopData::find_column("vaxstatus")->to_txt_cell(row2) == "booster");

    assert(PopData::find_column("variant")->to_txt_cell(row1) == "alpha");
    assert(PopData::find_column("variant")->to_txt_cell(row2) == "delta");
    assert(PopData::find_column("variant")->to_txt_cell(row3).empty());
    assert(PopData::find_column("variant_hist")->to_txt_cell(row2) == "alpha|delta");

    assert(PopData::find_column("sickday")->to_txt_cell(row1) == "2");
    assert(PopData::find_column("sickday")->to_txt_cell(row2) == "11");
    assert(PopData::find_column("sickday_hist")->to_txt_cell(row2) == "4|11");
    assert(PopData::find_column("recovday")->to_txt_cell(row1) == "9");
    assert(PopData::find_column("recovday")->to_txt_cell(row2).empty());
    assert(PopData::find_column("recovday_hist")->to_txt_cell(row1) == "9");
    assert(PopData::find_column("recovday_hist")->to_txt_cell(row2).empty());

    assert(PopData::find_column("testday_hist")->to_txt_cell(row2) == "6|12");
    assert(PopData::find_column("testday")->to_txt_cell(row2) == "12");

    assert(PopData::find_column("vax")->to_txt_cell(row2) == "moderna");
    assert(PopData::find_column("vax")->to_txt_cell(row3).empty());
    assert(PopData::find_column("vax_hist")->to_txt_cell(row2) == "pfizer|moderna");
    assert(PopData::find_column("vaxday")->to_txt_cell(row2) == "14");
    assert(PopData::find_column("vaxday_hist")->to_txt_cell(row2) == "5|14");

    fmt::println("=== Pop Column Registry Test Completed ===");
}

namespace pop_csv_test {
namespace fs = std::filesystem;

fs::path first_csv_in_dir(const fs::path& dir) {
  for (const auto& e : fs::directory_iterator(dir)) {
    if (e.is_regular_file() && e.path().extension() == ".csv") {
      return e.path();
    }
  }
  throw std::runtime_error("no csv in dir");
}

string read_file_text(const fs::path& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("cannot read csv");
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}
}  // namespace pop_csv_test

void test_popdata_csv_serialization_escape() {
  fmt::print("\n=== Testing PopData CSV escape ===\n\n");
  const vector<string> saved_names = Variant::names;
  Variant::names = {"none", "delta,epsilon"};
  PopData pop(1);
  pop.status[1] = UNEXPOSED;
  pop.cond[1] = UNINFECTED;
  pop.variant[1] = Variant{1};

  const pop_csv_test::fs::path out_dir =
      pop_csv_test::fs::temp_directory_path() /
      fmt::format("epi_sim_pop_csv_{}", std::random_device{}());
  pop_csv_test::fs::create_directories(out_dir);
  pop.serialize_selected_columns({"variant"}, "escape_test", {out_dir.string()});

  const pop_csv_test::fs::path csv_path = pop_csv_test::first_csv_in_dir(out_dir);
  const string content = pop_csv_test::read_file_text(csv_path);
  assert(content.find("\"delta,epsilon\"") != string::npos);
  Variant::names = saved_names;
  pop_csv_test::fs::remove_all(out_dir);
  fmt::println("=== PopData CSV escape test completed ===");
}

void test_popdata_csv_partial_and_dedupe_selections() {
  fmt::print("\n=== Testing PopData CSV partial / dedupe selections ===\n\n");
  PopData pop = poptable_test::make_popdata_print_fixture();

  const pop_csv_test::fs::path out_dir =
      pop_csv_test::fs::temp_directory_path() /
      fmt::format("epi_sim_pop_csv_sel_{}", std::random_device{}());
  pop_csv_test::fs::create_directories(out_dir);

  pop.serialize_selected_columns({"status", "not_a_column", "agegrp"}, "partial_test",
                                 {out_dir.string()});
  {
    const pop_csv_test::fs::path csv_path = pop_csv_test::first_csv_in_dir(out_dir);
    const string content = pop_csv_test::read_file_text(csv_path);
    const auto lines = poptable_test::split_trimmed_lines(content);
    assert(!lines.empty());
    assert(lines[0] == "row,status,agegrp");
  }
  for (const auto& e : pop_csv_test::fs::directory_iterator(out_dir)) {
    if (e.is_regular_file()) {
      pop_csv_test::fs::remove(e.path());
    }
  }

  pop.serialize_selected_columns({"status", "status", "agegrp"}, "dedupe_test", {out_dir.string()});
  {
    const pop_csv_test::fs::path csv_path = pop_csv_test::first_csv_in_dir(out_dir);
    const string content = pop_csv_test::read_file_text(csv_path);
    const auto lines = poptable_test::split_trimmed_lines(content);
    assert(lines[0] == "row,status,agegrp");
  }

  pop_csv_test::fs::remove_all(out_dir);
  fmt::println("=== PopData CSV partial / dedupe test completed ===");
}

void test_finalize_series() {
    fmt::print("\n=== Testing Finalize Series ===\n\n");

    HistorySeries series(4);
    auto& now_infected_total = series.at(SeriesName::now_infected, AgeBucket::total);
    auto& net_infected_total = series.at(SeriesName::net_infected, AgeBucket::total);
    auto& now_infected_40_59 = series.at(SeriesName::now_infected, AgeBucket::age40_59);
    auto& net_infected_40_59 = series.at(SeriesName::net_infected, AgeBucket::age40_59);

    now_infected_total[0] = 99;
    net_infected_total[0] = 77;
    now_infected_total[1] = 3;
    now_infected_total[2] = 5;
    now_infected_total[3] = 4;
    now_infected_total[4] = 9;

    now_infected_40_59[0] = 42;
    net_infected_40_59[0] = 24;
    now_infected_40_59[1] = 1;
    now_infected_40_59[2] = 1;
    now_infected_40_59[3] = 0;
    now_infected_40_59[4] = 4;

    series.finalize_series();

    const vector<int> expected_total = {77, 3, 2, -1, 5};
    const vector<int> expected_age = {24, 1, 0, -1, 4};

    assert(net_infected_total == expected_total);
    assert(net_infected_40_59 == expected_age);
    assert(now_infected_total[0] == 99);
    assert(now_infected_40_59[0] == 42);

    HistorySeries single_day_series(1);
    auto& single_now_infected_total = single_day_series.at(SeriesName::now_infected, AgeBucket::total);
    auto& single_net_infected_total = single_day_series.at(SeriesName::net_infected, AgeBucket::total);
    single_now_infected_total[0] = 11;
    single_net_infected_total[0] = 22;
    single_now_infected_total[1] = 6;

    single_day_series.finalize_series();

    assert(single_net_infected_total[0] == 22);
    assert(single_net_infected_total[1] == 6);

    fmt::println("=== Finalize Series Test Completed ===");
}

/*
void test_simple_plot_render() {
    fmt::print("\n=== Testing Plot HTML Render ===\n\n");

    const std::string title = "Unit Test Plot 2026";
    const std::string heading = "Simple plot render test";
    const std::vector<double> x = {0.0, 1.0, 2.5, 3.5};
    const std::vector<double> y = {0.0, 1.0, 6.25, 12.25};
    const auto output_path = write_plot(title, heading, x, y, "test_series");

    assert(std::filesystem::exists(output_path));
    assert(output_path.parent_path().filename() == "plot_output");
    const std::string filename = output_path.filename().string();
    assert(filename.starts_with("unit_test_plot_2026_"));
    assert(filename.ends_with(".html"));

    std::ifstream in(output_path);
    assert(in);

    const std::string html{
        std::istreambuf_iterator<char>(in),
        std::istreambuf_iterator<char>()};

    fmt::println("Rendered plot file: {}", output_path.string());
    fmt::println("Rendered HTML:");
    fmt::println("{}", html);

    assert(html.find("<title>Unit Test Plot 2026</title>") != std::string::npos);
    assert(html.find("<h2>Simple plot render test</h2>") != std::string::npos);
    assert(html.find("\"x\":[0.0,1.0,2.5,3.5]") != std::string::npos);
    assert(html.find("\"y\":[0.0,1.0,6.25,12.25]") != std::string::npos);
    assert(html.find("\"name\":\"test_series\"") != std::string::npos);
    assert(html.find("\"title\":\"Unit Test Plot 2026\"") != std::string::npos);
    assert(html.find("script id=\"plot-payload\" type=\"application/json\"") != std::string::npos);
    assert(html.find("JSON.parse(document.getElementById(\"plot-payload\").textContent)") != std::string::npos);
    assert(html.find("Plotly.newPlot(\"plot\", payload.data, payload.layout);") != std::string::npos);
    assert(html.find("{{TITLE}}") == std::string::npos);
    assert(html.find("{{HEADING}}") == std::string::npos);
    assert(html.find("{{PAYLOAD_JSON}}") == std::string::npos);

    std::filesystem::remove(output_path);

    fmt::println("=== Plot HTML Render Test Completed ===");
}
*/

/*
void test_plotly() {
    fmt::print("\n=== Testing Plotly Browser Launch ===\n\n");

    const std::string title = "Plotly Browser Test";
    const std::string heading =
        "If Plotly loaded correctly, this plot should open in your browser";
    const std::vector<double> x = {0.0, 1.0, 2.0, 3.0, 4.0};
    const std::vector<double> y = {0.0, 1.0, 4.0, 9.0, 16.0};

    const auto output_path = do_plot(title, heading, x, y, "quadratic");

    fmt::println("Opened plot file: {}", output_path.string());
    fmt::println("Verify in the browser that a line plot appears.");
    fmt::println("=== Plotly Browser Launch Test Completed ===");
}
*/

void test_sendrisk_indexing() {
    fmt::print("\n=== Testing Variant sendrisk Indexing ===\n\n");

    const vector<string> saved_variant_names = Variant::names;
    Variant::names.clear();

    nlohmann::ordered_json jdata = {
        {"base",
         {
             {"spread",
              {
                  {"sendrisk", {0.0, 0.3, 0.65, 0.75, 0.85}},
                  {"recvrisk", {0.1, 0.39, 0.44, 0.54, 0.56}},
              }},
             {"immunity",
              {
                  {"recovery_immunity", {{"base", 0.8}}},
                  {"immunehalflife", 360},
              }},
         }},
    };
    const auto [variants, infectparams] = load_variants_data(jdata);

    assert(variants.size() == infectparams.size());
    assert(variants.size() > 1);

    const auto& base_sendrisk = infectparams[1].sendrisk;
    assert(base_sendrisk.size() == 6);
    assert(approx_equal(base_sendrisk[0], 0.0, 1e-9));
    assert(approx_equal(base_sendrisk[1], 0.0, 1e-9));
    assert(approx_equal(base_sendrisk[2], 0.3, 1e-6));
    assert(approx_equal(base_sendrisk[5], 0.85, 1e-6));

    Variant::names = saved_variant_names;

    fmt::println("=== Variant sendrisk Indexing Test Completed ===");
}

void test_make_sick_and_seedcase_duration_indexing() {
    fmt::print("\n=== Testing make_sick and SeedCase Duration Indexing ===\n\n");

    parameter_test::VariantNamesGuard variant_names_guard;
    Variant::names = {"none", "base", "delta"};

    PopData pop(4);
    HistorySeries series(5);
    ModelParams mp;

    pop.agegrp[1] = AGE20_39;
    pop.agegrp[2] = AGE40_59;
    pop.agegrp[3] = AGE60_79;
    pop.agegrp[4] = AGE80_UP;

    sim::reset_day();
    sim::incr_day();
    sim::ds.day = sim::get_day();

    auto first_person = pop.agent(1);
    first_person.make_sick(Variant{1}, series);

    assert(first_person.status() == INFECTIOUS);
    assert(first_person.cond() == NIL);
    assert(first_person.duration() == 1);
    assert(first_person.variant_hist().count == 1);
    assert(first_person.variant() == Variant{1});
    assert(first_person.get_sickday() == 1);

    json seed_json = json::parse(R"([
        {
            "triggerday": 2,
            "startofday": true,
            "filter": [
                {"trait": "agegrp", "val": "age40_59"}
            ],
            "change": {
                "terms": [
                    {"trait": "status", "val": "infectious"},
                    {"trait": "cond", "val": "mild"},
                    {"trait": "duration", "val": 3},
                    {"trait": "variant", "val": "delta"}
                ],
                "count": 1
            }
        }
    ])");
    vector<SeedCase> seed_cases = load_seed_cases(seed_json, pop, mp);
    assert(seed_cases.size() == 1);

    sim::incr_day();
    sim::ds.day = sim::get_day();
    const auto seeded = seed_cases.front()(series);

    assert(seeded.size() == 1);
    assert(seeded.front() == 2);
    const auto seeded_person = pop.agent(seeded.front());
    assert(seeded_person.status() == INFECTIOUS);
    assert(seeded_person.cond() == MILD);
    assert(seeded_person.duration() == 3);
    assert(seeded_person.variant_hist().count == 1);
    assert(seeded_person.variant() == Variant{2});
    assert(seeded_person.variant_hist().latest() == Variant{2});
    assert(seeded_person.get_sickday() == 2);

    fmt::println("=== make_sick and SeedCase Duration Indexing Test Completed ===");
}

void test_seedcase_infectious_change_requires_variant() {
    fmt::print("\n=== Testing Infectious SeedCase Requires Variant ===\n\n");

    parameter_test::VariantNamesGuard variant_names_guard;
    Variant::names = {"none", "base", "delta"};

    PopData pop(2);
    HistorySeries series(5);

    pop.agegrp[1] = AGE20_39;
    pop.agegrp[2] = AGE40_59;

    Filter filter{{{"agegrp", int32_t(uint8_t(AGE20_39))}}};
    Change change{{{"status", int32_t(uint8_t(INFECTIOUS))},
                   {"cond", int32_t(uint8_t(MILD))},
                   {"duration", 3}},
                  1};
    SeedCase seed_case(1, true, filter, change, pop);

    bool threw = false;
    try {
        seed_case(series);
    } catch (const std::runtime_error& ex) {
        threw = true;
        assert(string(ex.what()).find("variant") != string::npos);
    }

    assert(threw);
    assert(pop.agent(1).status() == UNEXPOSED);
    assert(pop.agent(1).variant() == Variant{0});
    assert(pop.agent(1).variant_hist().count == 0);

    fmt::println("=== Infectious SeedCase Requires Variant Test Completed ===");
}

void test_make_well_recovday_history() {
    fmt::print("\n=== Testing make_well Recovery Day History ===\n\n");

    PopData pop(1);
    HistorySeries series(20);
    auto person = pop.agent(1);

    const vector<string> saved_variant_names = Variant::names;
    Variant::names = {"none", "base"};
    person.variant() = Variant{1};

    sim::reset_day();

    for (int day = 1; day <= 17; ++day) {
        sim::incr_day();
        sim::ds.day = sim::get_day();

        person.status() = INFECTIOUS;
        person.cond() = MILD;
        person.duration() = 3;
        person.make_well(series);
    }

    assert(person.status() == RECOVERED);
    assert(person.cond() == UNINFECTED);
    assert(person.duration() == 0);
    assert(person.recovday() == 17);
    assert(person.recovday_hist().count == 17);
    assert(person.recovday_hist().stored_count() == 16);
    assert(person.recovday_hist().arr[0] == 2);
    assert(person.recovday_hist().arr[15] == 17);
    assert(person.recovday_hist().latest() == 17);
    assert(person.recovday_hist().show() == "2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17");

    Variant::names = saved_variant_names;

    fmt::println("=== make_well Recovery Day History Test Completed ===");
}

void test_testday_history_and_derived_tested() {
    fmt::print("\n=== Testing testday History and Derived tested ===\n\n");

    PopData pop(2);
    HistorySeries series(5);

    assert(pop.testday[1] == 0);
    assert(pop.testday_hist[1].count == 0);
    for (int day = 1; day <= 17; ++day) {
        pop.testday_hist[1].set(day);
    }
    pop.testday[1] = 17;

    assert(pop.testday_hist[1].stored_count() == 16);
    assert(pop.testday_hist[1].arr[0] == 2);
    assert(pop.testday_hist[1].arr[15] == 17);
    assert(pop.testday_hist[1].latest() == 17);
    assert(pop.testday[1] != 0);

    Filter filter{{{"tested", 1}}};
    Change change{{{"quar", 1}}, 1};
    SeedCase seed_case(1, true, filter, change, pop);

    const auto seeded = seed_case(series);
    assert(seeded.size() == 1);
    assert(seeded.front() == 1);
    assert(pop.quar[1] == 1);
    assert(pop.quar[2] == 0);

    fmt::println("=== testday History and Derived tested Test Completed ===");
}

void test_recoveffect_uses_scalar_recovday() {
    fmt::print("\n=== Testing recoveffect Scalar recovday ===\n\n");

    PopData pop(2);
    pop.variant[1] = Variant{1};
    pop.recovday[1] = 10;
    pop.recovday_hist[1].count = 0;
    pop.variant[2] = Variant{1};
    pop.recovday[2] = 0;

    vector<InfectParams> infectparams(2);
    infectparams[1].recovery_immunity = {0.0f, 0.5f};
    infectparams[1].immunehalflife = 120;

    const float recovered_factor = recoveffect(pop.agent(1), 20, 1, infectparams);
    const float naive_factor = recoveffect(pop.agent(2), 20, 1, infectparams);

    assert(recovered_factor < 1.0f);
    assert(recovered_factor >= 0.0f);
    assert(approx_equal(naive_factor, 1.0f, 1e-6));

    fmt::println("=== recoveffect Scalar recovday Test Completed ===");
}

void test_vaccinate_uses_scalar_recovday() {
    fmt::print("\n=== Testing vaccinate Scalar recovday Eligibility ===\n\n");
    parameter_test::VaxNamesGuard vax_names_guard;
    Vax::names = {"none", "pfizer"};

    PopData pop(3);
    for (int p = 1; p <= 3; ++p) {
        pop.agegrp[p] = AGE20_39;
    }

    pop.status[1] = UNEXPOSED;
    pop.status[2] = RECOVERED;
    pop.recovday[2] = 5;
    pop.status[3] = RECOVERED;
    pop.recovday[3] = 10;

    VaxParams pfizer;
    pfizer.reqdshots = 1;
    pfizer.delay2ndshot = 0;
    pfizer.delaybooster = 999;

    VaxSet vaxset;
    vaxset.params.push_back(VaxParams{});
    vaxset.params.push_back(pfizer);

    PerVaxSpec spec;
    spec.vax = Vax{1};
    spec.mix = 1.0f;
    spec.starting_doses = 10;
    spec.doses = 10;
    spec.pct2ndshot = 1.0f;
    spec.pctboost = 1.0f;

    VaxSched sched;
    sched.vaxesincluded.push_back(spec);
    sched.dayrange = {1, 30};
    sched.targetpct = 1.0f;
    sched.filtervec = {AGE20_39};
    sched.shotmode = "all";
    sched.pattern = {1.0f, 1.0f};
    sched.spreadfunc = [](int) { return 1.0f; };

    VaxSchedSet schedset;
    schedset.schedules.push_back({"unit_test_sched", sched});

    HistorySeries series(30);
    vaccinate(20, schedset, vaxset, pop, series);

    assert(pop.vax_hist[1].count == 1);
    assert(pop.vax_hist[2].count == 1);
    assert(pop.vax_hist[3].count == 0);
    assert(pop.vax[1] == Vax{1});
    assert(pop.vax[2] == Vax{1});
    assert(idx(pop.vax[3]) == 0);

    fmt::println("=== vaccinate Scalar recovday Eligibility Test Completed ===");
}

void test_vaxeffect_uses_scalar_latest_vax() {
    fmt::print("\n=== Testing vaxeffect Scalar latest vaccine/day ===\n\n");
    parameter_test::VariantNamesGuard variant_names_guard;
    parameter_test::VaxNamesGuard vax_names_guard;
    Variant::names = {"none", "base"};
    Vax::names = {"none", "pfizer"};

    PopData pop(2);
    pop.vaxstatus[1] = Vaxstat::full;
    pop.vax[1] = Vax{1};
    pop.vaxday[1] = 10;
    pop.vax_hist[1].count = 0;
    pop.vaxday_hist[1].count = 0;

    VaxParams pfizer;
    pfizer.halflife = 180;
    pfizer.full_effect_days = 14;
    pfizer.day1_effect = 0.65f;
    pfizer.infectfactor = {{"base", 0.9f}};
    pfizer.effectiveness = {
        {"first", {{"base", 0.7f}}},
        {"full", {{"base", 0.9f}}},
        {"booster", {{"base", 0.95f}}},
    };

    VaxSet vaxset;
    vaxset.params.push_back(VaxParams{});
    vaxset.params.push_back(pfizer);

    const float protected_factor = vaxeffect(20, pop.agent(1), vaxset, 1);
    const float naive_factor = vaxeffect(20, pop.agent(2), vaxset, 1);

    assert(protected_factor < 1.0f);
    assert(protected_factor >= 0.0f);
    assert(approx_equal(naive_factor, 1.0f, 1e-6));

    fmt::println("=== vaxeffect Scalar latest vaccine/day Test Completed ===");
}

void test_vax_history_overflow() {
    fmt::print("\n=== Testing vax history overflow retention ===\n\n");
    parameter_test::VaxNamesGuard vax_names_guard;
    Vax::names = {"none", "pfizer"};

    PopData pop(1);

    for (int day = 1; day <= 17; ++day) {
        pop.vax[1] = Vax{1};
        pop.vaxday[1] = static_cast<int16_t>(day);
        pop.vax_hist[1].set(Vax{1});
        pop.vaxday_hist[1].set(static_cast<int16_t>(day));
    }

    assert(pop.vax[1] == Vax{1});
    assert(pop.vaxday[1] == 17);
    assert(pop.vax_hist[1].count == 17);
    assert(pop.vax_hist[1].stored_count() == 16);
    assert(pop.vax_hist[1].latest() == Vax{1});
    assert(pop.vaxday_hist[1].count == 17);
    assert(pop.vaxday_hist[1].stored_count() == 16);
    assert(pop.vaxday_hist[1].arr[0] == 2);
    assert(pop.vaxday_hist[1].arr[15] == 17);
    assert(pop.vaxday_hist[1].latest() == 17);

    fmt::println("=== vax history overflow retention Test Completed ===");
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

void test_model_params() {
  fmt::print("\n=== Testing ModelParams Loading ===\n\n");

  parameter_test::VariantNamesGuard variant_names_guard;
  parameter_test::VaxNamesGuard vax_names_guard;
  Variant::names.clear();
  Vax::names.clear();
  const auto paths = parameter_test::sample_paths();

  GeoData geodata = load_geodata_csv(paths.geodata);
  assert(geodata.num_rows > 0);
  assert(geodata.num_rows == geodata.fips.size());
  assert(geodata.num_rows == geodata.county.size());
  assert(geodata.num_rows == geodata.pop.size());
  const size_t locale_idx = parameter_test::require_locale_index(geodata, 38015);
  assert(geodata.county[locale_idx] == "Burleigh");
  assert(geodata.city[locale_idx] == "Bismarck");
  assert(geodata.state[locale_idx] == "ND");
  assert(geodata.pop[locale_idx] == 95626);
  assert(geodata.indoor_st[locale_idx] == "0001-09-15");
  assert(geodata.indoor_end[locale_idx] == "0002-05-30");

  auto [infectparams, progressionset, trvec, variants] = load_infect_params(paths.variants);
  assert(!variants.empty());
  assert(variants.size() == infectparams.size());
  assert(variants.size() == progressionset.progression.size());
  assert(variants[0].show() == "none");
  assert(variants[1].show() == "base");
  assert(variants[2].show() == "alpha");
  assert(infectparams[1].sendrisk.size() > 20);
  assert(approx_equal(infectparams[1].sendrisk[1], 0.0, 1e-9));
  assert(approx_equal(infectparams[1].sendrisk[2], 0.3, 1e-9));
  assert(approx_equal(infectparams[1].sendrisk[5], 0.85, 1e-9));
  assert(approx_equal(infectparams[1].recvrisk[0], 0.1, 1e-9));
  assert(approx_equal(infectparams[1].recvrisk[4], 0.56, 1e-9));
  assert(progressionset.progression[1].tree.size() == 5);
  assert(progressionset.progression[1].tree[0].contains(5));
  const auto& base_age0_day5_nil = progressionset.progression[1].tree[0].at(5)[0];
  assert(base_age0_day5_nil.size() == 6);
  assert(approx_equal(base_age0_day5_nil[1], 0.4, 1e-9));
  assert(approx_equal(base_age0_day5_nil[2], 0.5, 1e-9));
  assert(progressionset.progression[2].factors.riskadjust.size() == 6);
  assert(approx_equal(progressionset.progression[2].factors.riskadjust[3], 1.1, 1e-9));
  assert(std::all_of(trvec.begin(), trvec.end(),
                     [](float value) { return approx_equal(value, 0.0, 1e-9); }));

  SocialParams socialdata = load_social_params(paths.social);
  assert(approx_equal(socialdata.gammashape, 1.0, 1e-9));
  assert(approx_equal(socialdata.indoor_uplift, 1.1, 1e-9));
  assert(approx_equal(socialdata.contactfactors[0][1], 1.995, 1e-9));
  assert(approx_equal(socialdata.contactfactors[3][4], 0.475, 1e-9));
  assert(approx_equal(socialdata.touchfactors[0][0], 0.55, 1e-9));
  assert(approx_equal(socialdata.touchfactors[4][0], 0.28, 1e-9));

  VaxSet vaxset = load_vax_data(paths.vaccines);
  assert(vaxset.size() == 3);
  assert(Vax::names.size() == 4);
  assert(Vax::names[0] == "none");
  assert(Vax::names[1] == "Pfizer");
  assert(Vax::names[2] == "Moderna");
  assert(Vax::names[3] == "JnJ");
  const auto& pfizer = parameter_test::require_vax(vaxset, "Pfizer");
  assert(pfizer.reqdshots == 2);
  assert(pfizer.delay2ndshot == 21);
  assert(pfizer.delaybooster == 160);
  assert(approx_equal(pfizer.day1_effect, 0.65, 1e-9));
  assert(approx_equal(parameter_test::require_named_factor(pfizer.infectfactor, "delta"), 0.85, 1e-9));
  assert(pfizer.effectiveness.size() == 3);
  assert(pfizer.effectiveness[1].first == "full");
  assert(approx_equal(parameter_test::require_named_factor(pfizer.effectiveness[1].second, "delta"), 0.8, 1e-9));

  VaxSchedSet vaxschedset = load_vax_sched_set(paths.vax_sched_dir);
  assert(vaxschedset.size() == 2);

  const auto& old_sched = parameter_test::require_sched(vaxschedset, "loc38015_old");
  assert(old_sched.vaxesincluded.size() == 3);
  const auto& jnj_sched = parameter_test::require_sched_vax(old_sched, "JnJ");
  assert(approx_equal(jnj_sched.mix, 0.05, 1e-9));
  assert(jnj_sched.starting_doses == 4000);
  assert(approx_equal(jnj_sched.pct2ndshot, 0.0, 1e-9));
  assert(approx_equal(jnj_sched.pctboost, 0.4, 1e-9));
  assert(old_sched.dayrange.first == 350);
  assert(old_sched.dayrange.second == 700);
  assert(approx_equal(old_sched.targetpct, 0.95, 1e-9));
  assert(old_sched.filtervec.size() == 2);
  assert(old_sched.filtervec[0] == AGE80_UP);
  assert(old_sched.filtervec[1] == AGE60_79);
  assert(old_sched.shotmode == "all");
  assert(old_sched.pattern.size() == 12);
  assert(approx_equal(old_sched.pattern[3], 0.1, 1e-9));
  assert(static_cast<bool>(old_sched.spreadfunc));

  const auto& young_sched = parameter_test::require_sched(vaxschedset, "loc38015_young");
  assert(approx_equal(young_sched.targetpct, 0.65, 1e-9));
  assert(young_sched.filtervec.size() == 3);
  assert(young_sched.filtervec[0] == AGE0_19);
  assert(young_sched.filtervec[1] == AGE20_39);
  assert(young_sched.filtervec[2] == AGE40_59);
  assert(static_cast<bool>(young_sched.spreadfunc));

  ModelParams mp{
      .geodata = std::move(geodata),
      .variants = std::move(variants),
      .infectparams = std::move(infectparams),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = std::move(socialdata),
      .vaxset = std::move(vaxset),
      .vaxschedset = std::move(vaxschedset),
  };

  assert(mp.geodata.pop[locale_idx] == 95626);
  assert(mp.variants[1].show() == "base");
  assert(mp.vaxset.size() == 3);
  assert(mp.vaxschedset.size() == 2);

  fmt::println("==================== Model Parameters ==================");
  mp.geodata.print();
  fmt::print("\n");
  for (size_t i = 0; i < mp.variants.size(); ++i) {
    fmt::print("{:>2}: {}\n", i, mp.variants[i].show());
  }
  fmt::print("\n");
  print_infectparams(mp.infectparams, mp.variants);
  fmt::print("\n");
  mp.socialdata.print();
  fmt::print("\n");
  mp.progressionset.print(mp.variants);
  fmt::print("\n");
  mp.vaxset.print();
  fmt::print("\n");
  mp.vaxschedset.print();
  fmt::print("\n");

  fmt::println("=== ModelParams Loading Test Completed ===");
}

// // This test depends on successful parameter loading because setup_sim builds ModelParams first.
// void test_build_model() {
//   fmt::print("\n=== Testing Model Build ===\n\n");

//   parameter_test::VariantNamesGuard variant_names_guard;
//   Variant::names.clear();

//   constexpr int ndays = 366;
//   Model model = setup_sim(ndays, 38015, "2020-01-01", false);

//   assert(model.ndays == ndays);
//   assert(model.locale == 38015);
//   assert(model.caldays.size() == size_t(ndays));
//   assert(model.indoor_seq.size() == size_t(ndays));
//   assert(model.day1 == absl::CivilDay(2020, 1, 1));
//   assert(model.caldays.front() == absl::CivilDay(2020, 1, 1));
//   assert(model.caldays.back() == absl::CivilDay(2020, 12, 31));

//   const size_t jan1_idx = parameter_test::require_calday_index(model.caldays, absl::CivilDay(2020, 1, 1));
//   const size_t may30_idx = parameter_test::require_calday_index(model.caldays, absl::CivilDay(2020, 5, 30));
//   const size_t jun1_idx = parameter_test::require_calday_index(model.caldays, absl::CivilDay(2020, 6, 1));
//   const size_t sep15_idx = parameter_test::require_calday_index(model.caldays, absl::CivilDay(2020, 9, 15));
//   const size_t dec31_idx = parameter_test::require_calday_index(model.caldays, absl::CivilDay(2020, 12, 31));
//   assert(approx_equal(model.indoor_seq[jan1_idx], 1.1, 1e-9));
//   assert(approx_equal(model.indoor_seq[may30_idx], 1.1, 1e-9));
//   assert(approx_equal(model.indoor_seq[jun1_idx], 1.0, 1e-9));
//   assert(approx_equal(model.indoor_seq[sep15_idx], 1.1, 1e-9));
//   assert(approx_equal(model.indoor_seq[dec31_idx], 1.1, 1e-9));

//   const size_t locale_idx = parameter_test::require_locale_index(model.mp.geodata, model.locale);
//   assert(model.pop.popn == model.mp.geodata.pop[locale_idx]);
//   assert(model.pop.popz == model.pop.popn + 1);
//   assert(model.mp.variants[1].name() == "base");

//   fmt::println("=== Model Build Test Completed ===");
// }

// void test_popdata_size(Model model) {
//   fmt::println("Vector sizing: {} popz {}", model.pop.status.size(), model.pop.popz);
//   fmt::println("Index to actual population size: {}",
//                Agegrp::names[model.pop.agegrp[model.pop.popn]]);
// }
// 
// void test_multiple_infections() {
//   fmt::print("\n=== Testing Multiple Infections Over Time ===\n\n");

//   // Create a fresh model for this test
//   fmt::println("Setting up independent test environment...");
//   Model test_model = setup_sim(1000, 38015, "2020-01-01", false);
//   PopData& pop = test_model.pop;
//   vector<Variant>& variants = test_model.mp.variants;

//   // Find one person in age20_39 and one in age60_79
//   size_t idx_age20_39 = 0;
//   size_t idx_age60_79 = 0;

//   for (size_t i = 1; i <= pop.popn; ++i) {
//     if (idx_age20_39 == 0 && pop.agegrp[i] == Age::Age20_39) {
//       idx_age20_39 = i;
//     }
//     if (idx_age60_79 == 0 && pop.agegrp[i] == Age::Age60_79) {
//       idx_age60_79 = i;
//     }
//     if (idx_age20_39 != 0 && idx_age60_79 != 0) {
//       break;
//     }
//   }

//   if (idx_age20_39 == 0 || idx_age60_79 == 0) {
//     fmt::println("ERROR: Could not find required age groups in population");
//     return;
//   }

//   // resolve idx for person to the person's row of traits:
//   auto person_age20_39 = pop.agent(idx_age20_39);
//   auto person_age60_79 = pop.agent(idx_age60_79);

//   fmt::println("Testing with:");
//   fmt::println("  Person {} (age group: {}) - will be infected 10 times",
//                person_age20_39.id, Agegrp::names[person_age20_39.agegrp()]);
//   fmt::println("  Person {} (age group: {}) - will be infected 17 times\n",
//                person_age60_79.id, Agegrp::names[person_age60_79.agegrp()]);

//   // Use first real variant (index 1, since 0 is "none")
//   Variant base_variant = test_model.mp.variants[1];
//   Condition condition = Cond::Nil;
//   uint8_t duration = 5;

//   // Reset sim day to 1
//   sim::current_day = 1;
//   HistorySeries series(400);

//   // Infect person 1 ten times (days 1, 21, 41, 61, 81, 101, 121, 141, 161, 181)
//   fmt::println("Infecting person {} 10 times:", person_age20_39.id);
//   for (int infection = 0; infection < 10; ++infection) {
//     int day = 1 + (infection * 20);
//     sim::current_day = day;
//     person_age20_39.make_sick(base_variant, series, condition, duration);
//     fmt::println("  Infection {} on day {} - variant_count: {}",
//                  infection + 1, day, static_cast<int>(person_age20_39.variant_count()));
//   }

//   fmt::println("\nPerson {} final state:", person_age20_39.id);
//   fmt::println("  variant_count: {}", static_cast<int>(person_age20_39.variant_count()));
//   fmt::println("  Infection history (variant, sickday):");
//   int count1 = std::min(static_cast<int>(person_age20_39.variant_count()), 16);
//   for (int i = 0; i < count1; ++i) {
//     fmt::println("    [{}] variant: {}, sickday: {}",
//                  i, person_age20_39.get_variant().name(), person_age20_39.get_sickday());
//   }

//   // Infect person 2 seventeen times (days 1, 21, 41, ..., 321)
//   fmt::print("\n\n");
//   fmt::println("Infecting person {} 17 times:", person_age60_79.id);
//   for (int infection = 0; infection < 17; ++infection) {
//     int day = 1 + (infection * 20);
//     sim::current_day = day;
//     person_age60_79.make_sick(base_variant, series, condition, duration);
//     fmt::println("  Infection {} on day {} - variant_count: {}",
//                  infection + 1, day, static_cast<int>(person_age60_79.variant_count()));
//   }

//   fmt::println("\nPerson {} final state:", person_age60_79.id);
//   fmt::println("  variant_count: {}", static_cast<int>(person_age60_79.variant_count()));
//   fmt::println("  Infection history (variant, sickday):");
//   int count2 = std::min(static_cast<int>(person_age60_79.variant_count()), 16);
//   fmt::println("  (Showing {} most recent infections, max capacity is 16)", count2);
//   for (int i = 0; i < count2; ++i) {
//     fmt::println("    [{}] variant: {}, sickday: {}",
//                  i, person_age60_79.get_variant().name(), person_age60_79.get_sickday());
//   }

//   fmt::println("\n=== Multiple Infections Test Completed ===");
// }

// void test_seedcase_multiple_infections() {
//   fmt::print("\n=== Testing SeedCase with Multiple Infections ===\n\n");

//   // Create a fresh model for this test
//   fmt::println("Setting up independent test environment...");
//   Model test_model = setup_sim(1000, 38015, "2020-01-01", false);
//   PopData& pop = test_model.pop;
//   HistorySeries series(321);

//   // Use first real variant (index 1, since 0 is "none")
//   Variant base_variant = test_model.mp.variants[1];
//   Condition condition = Cond::Nil;
//   uint8_t duration = 5;

//   // Create SeedCases:
//   // - One for age20_39 that triggers 10 times (days 1, 21, 41, ..., 181)
//   // - One for age60_79 that triggers 17 times (days 1, 21, 41, ..., 321)

//   std::vector<SeedCase> seed_cases;

//   // Create 10 seed cases for age20_39 (one person each time)
//   for (int i = 0; i < 10; ++i) {
//     int trigger_day = 1 + (i * 20);
//     std::vector<SeedFilter> filters{{Age::Age20_39, condition, 0, base_variant, 1}};
//     seed_cases.emplace_back(trigger_day, true, filters, pop);
//   }

//   // Create 17 seed cases for age60_79 (one person each time)
//   for (int i = 0; i < 17; ++i) {
//     int trigger_day = 1 + (i * 20);
//     std::vector<SeedFilter> filters{{Age::Age60_79, condition, 0, base_variant, 1}};
//     seed_cases.emplace_back(trigger_day, true, filters, pop);
//   }

//   fmt::println("Created {} seed cases", seed_cases.size());
//   fmt::println("  10 for age20_39 (days 1, 21, 41, ..., 181)");
//   fmt::println("  17 for age60_79 (days 1, 21, 41, ..., 321)\n");

//   // Track which persons get infected
//   size_t person_age20_39 = 0;
//   size_t person_age60_79 = 0;

//   // Simulate days 1 through 321, triggering seed cases as appropriate
//   fmt::println("Running simulation through day 321...");
//   for (int day = 1; day <= 321; ++day) {
//     sim::current_day = day;

//     // Check each seed case to see if it should trigger today
//     for (auto& seed_case : seed_cases) {
//       if (seed_case.triggerday == day) {
//         auto seeded = seed_case(series);

//         // Track which persons were seeded
//         if (!seeded.empty()) {
//           size_t person = seeded[0];
//           if (pop.agegrp[person] == Age::Age20_39) {
//             person_age20_39 = person;
//             fmt::println("  Day {}: Infected person {} (age20_39), variant_count now: {}",
//                          day, person, static_cast<int>(pop.variant_count[person]));
//           } else if (pop.agegrp[person] == Age::Age60_79) {
//             person_age60_79 = person;
//             fmt::println("  Day {}: Infected person {} (age60_79), variant_count now: {}",
//                          day, person, static_cast<int>(pop.variant_count[person]));
//           }
//         }
//       }
//     }
//   }

//   // Display results
//   if (person_age20_39 == 0 || person_age60_79 == 0) {
//     fmt::println("\nERROR: Not all persons were infected");
//     return;
//   }

//   fmt::print("\n=== Final Results ===\n\n");

//   fmt::println("Person {} (age20_39) final state:", person_age20_39);
//   fmt::println("  variant_count: {}", static_cast<int>(pop.variant_count[person_age20_39]));
//   fmt::println("  Infection history (variant, sickday):");
//   int count1 = std::min(static_cast<int>(pop.variant_count[person_age20_39]), 16);
//   for (int i = 0; i < count1; ++i) {
//     fmt::println("    [{}] variant: {}, sickday: {}",
//                  i, pop.variant[person_age20_39][i].name(), pop.sickday[person_age20_39][i]);
//   }

//   fmt::println("\nPerson {} (age60_79) final state:", person_age60_79);
//   fmt::println("  variant_count: {}", static_cast<int>(pop.variant_count[person_age60_79]));
//   fmt::println("  Infection history (variant, sickday):");
//   int count2 = std::min(static_cast<int>(pop.variant_count[person_age60_79]), 16);
//   fmt::println("  (Showing {} most recent infections, max capacity is 16)", count2);
//   for (int i = 0; i < count2; ++i) {
//     fmt::println("    [{}] variant: {}, sickday: {}",
//                  i, pop.variant[person_age60_79][i].name(), pop.sickday[person_age60_79][i]);
//   }

//   fmt::println("\n=== SeedCase Multiple Infections Test Completed ===");
// }

// void test_seeding_and_spread() {
//   fmt::print("\n=== Integration Test: Seeding and Spread Contact Generation ===\n\n");

//   // Setup simulation - this creates the model with real parameters
//   fmt::println("Setting up simulation with locale 38015, for 3 days...");
//   Model model = setup_sim(3, 38015, "2020-01-01", false);

//   fmt::println("Population size: {}", model.pop.popn);
//   fmt::println("Number of days: {}", model.ndays);
//   fmt::println("Start date: {}\n", absl::FormatCivilTime(model.day1));

//   fmt::println("--- Running Simulation ---");
//   fmt::println("This will use the SeedCases defined in sim.cpp");
//   fmt::println("Expected: 6 people infected (3 age20_39 + 3 age40_59) with duration=5");
//   fmt::println("The spread() function will print contacts for each infectious person\n");

//   // Run the actual simulation - this uses the real SeedCases from sim.cpp
//   runsim(model);

//   fmt::println("\n=== Integration Test Completed ===");
// }

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

// // needs more inputs to work...
// void apportion_debug(int n, vector<float> splits) {
//         fmt::println("\n=== APPORTION DEBUG (n={}) ===", n);

//       vector<int> parts;
//       for (size_t i = 0; i < splits.size(); ++i) {
//         int part = static_cast<int>(round(n * splits[i]));
//         parts.push_back(part);
//         fmt::println("  parts[{}] = round({} * {}) = {}", i, n, splits[i], part);
//       }

//       // Calculate total before adjustment
//       int total_before = std::accumulate(parts.begin(), parts.end(), 0);
//       fmt::println("Total before adjustment: {}", total_before);

//       // fix rounding error
//       int diff = total_before - n;
//       fmt::println("Difference (total - n): {}", diff);

//       if (diff != 0) {
//         fmt::println("Adjusting parts.back() from {} to {}", parts.back(), parts.back() - diff);
//         parts.back() -= diff;
//       }

//       // Calculate total after adjustment
//       int total_after = std::accumulate(parts.begin(), parts.end(), 0);
//       fmt::println("Total after adjustment: {}", total_after);
//       fmt::println("Expected (n): {}", n);
//       fmt::println("Match: {}", total_after == n ? "YES" : "NO");
//       fmt::println("=== END APPORTION DEBUG ===\n");
// }

// void sim_test(size_t ndays=180, int locale=38015) {
//   // fmt::print("\n=== Testing Spread Function (180-day simulation) ===\n\n");
//   parameter_test::VariantNamesGuard variant_names_guard;
//   Variant::names.clear();
//   Model model = setup_sim(ndays, locale, "2020-01-01", false);
//   // fmt::println("Population: {}", model.pop.popn);
//   // fmt::println("Running simulation for {} days...\n", model.ndays);

//   runsim(model);
 
// }

// void test_short_sim_smoke(int days) {
//   fmt::print("\n=== Smoke Test: Short Simulation Run ===\n\n");
//   sim_test(days, 38015);
//   fmt::println("=== Short Simulation Smoke Test Completed ===");
// }

int main() {
  test_primitive_column_wrappers();
  test_pop_column_registry();
  test_popdata_csv_serialization_escape();
  test_popdata_csv_partial_and_dedupe_selections();
  test_agent_pop_print();
  test_make_sick_and_seedcase_duration_indexing();
  test_seedcase_infectious_change_requires_variant();
  test_make_well_recovday_history();
  test_testday_history_and_derived_tested();
  test_recoveffect_uses_scalar_recovday();
  test_vaccinate_uses_scalar_recovday();
  test_vaxeffect_uses_scalar_latest_vax();
  test_vax_history_overflow();
  test_model_params();
  // test_build_model();
  // test_finalize_series();
  // test_simple_plot_render();
  // test_sendrisk_indexing();
  // test_short_sim_smoke(180);
}
