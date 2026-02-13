#pragma once

#include <csv2/reader.hpp> 
#include <string>
#include <algorithm>
#include <vector>
#include <array>
// #include <iostream>
// #include <fstream>
// #include <iomanip>
#include <cstdlib>
// #include <unordered_map>
#include "absl/container/flat_hash_map.h"
#include <tuple>
#include <utility>
// #include <yaml-cpp/node/parse.h>
// #include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>  // amazing for parsing complex files (maybe not for high speed web services)
// #include <fmt/base.h>
#include <fmt/format.h>  // only get what I use: about 12k in the executable!
#include <fmt/ranges.h>  // for printing containers like vector

#include "helpers.h"    // for shifter

// using json = nlohmann::json;
using json = nlohmann::ordered_json;
using std::array;
using std::string;
using std::vector;

//
// forward declarations of essential variables used to build structs
//


struct GeoData {
    // Column metadata
    vector<string> column_names = {
        "fips", "county", "city", "state", "sizecat",
        "pop", "density", "anchor", "indoor_st", "indoor_end"
    };

    size_t num_rows = 0;

    // Typed data vectors
    vector<int> fips;
    vector<string> county;
    vector<string> city;
    vector<string> state;
    vector<int> sizecat;
    vector<int> pop;
    vector<float> density;
    vector<string> anchor;      // Date stored as string (could parse to date type)
    vector<string> indoor_st;   // Date stored as string
    vector<string> indoor_end;  // Date stored as string

    // Helper to get type name for a column
    std::string get_type(const std::string& col_name) const {
        if (col_name == "fips" || col_name == "sizecat" ||
            col_name == "pop") return "int";

        if (col_name == "density") return "float";
        return "string";
    }

    void print() {
      // Header
      fmt::print("{:<4}{:<8}{:<18}{:<18}{:<8}{:<10}{:<10}{:<10}{:<12}\n",
                "", "fips", "county", "city", "state",
                "sizecat", "pop", "density", "anchor");

      // Rows
      for (size_t i = 0; i < num_rows; ++i) {
          fmt::print("{:>2}: {:<8}{:<18}{:<18}{:<8}{:<10}{:<10}{:<10.3}{:<12}\n",
              //  fmt::format("{}:", i),
              i,
                fips[i],
                county[i],
                city[i],
                state[i],
                sizecat[i],
                pop[i],
                density[i],
                anchor[i]);
      }
    }    
};

struct RuntimeEnum {
  std::vector<std::string> names; // Index-to-Name (Number -> String)
  absl::flat_hash_map<std::string, int>
      lookup; // Name-to-Index (String -> Number)
  std::vector<uint8_t> valid_nums{};
  std::uint8_t nextnum{0};

  void add_item(std::string newname) {
      if (lookup.find(newname) == lookup.end()) { // Avoid duplicates
          names.push_back(newname);
          lookup[newname] = nextnum;
          nextnum++;
      }
  }

    // String → Index (linear search - fast for small N)
  int operator()(const string& name) const {
      auto it = std::find(names.begin(), names.end(), name);
      return (it != names.end()) ? std::distance(names.begin(), it) : 99;
  }

  // String → Index (hash map - for large N or explicit use)
  int to_int(const std::string& name) const {
      auto it = lookup.find(name);
      return (it != lookup.end()) ? it->second : 99;
  }

  // Num -> String (Instant)
  std::string to_str(uint8_t i) const {
    if (names.empty()) return std::to_string(i);
    return (i >= 0 && i < names.size()) ? names[i] : "INVALID";
  }

  size_t size() const { return names.size(); }

  void print() const {
    for (size_t i = 0; i < names.size(); ++i) {
      fmt::print("{:>2}: {:<8}\n", i, names[i]);
    }
  }
};

// "fake" enums created at runtime to hold characteristics of persons in the simulation
//     in the PopData table
namespace Traits
{
  inline RuntimeEnum Justint{};

  inline RuntimeEnum true_false = {{"true", "false"}, {{"true", 0}, {"false", 1}}, {0,1}, 2};

  inline RuntimeEnum Condition = {
      {"uninfected", "nil", "mild", "sick", "severe"}, // names
      {{"uninfected", 0},
        {"nil", 1},
        {"mild", 2},
        {"sick", 3},
        {"severe", 4}}, // lookup
      {0,1,2,3,4},      // valid_nums
      5};

  inline RuntimeEnum Status = {
      {"none", "unexposed", "infectious", "recovered", "dead"}, // names
      {{"none", 0},
        {"unexposed", 1},
        {"infectious", 2},
        {"recovered", 3},
        {"dead", 4}}, // lookup
      {1,2,3,4},  // valid_nums
        5};

  inline RuntimeEnum Agegrp = {
      {"unknown", "age0_19", "age20_39", "age40_59", "age60_79",
        "age80_up"}, // names
      {{"unknown", 0},
          {"age0_19", 1},
          {"age20_39", 2},
          {"age40_59", 3},
          {"age60_79", 4},
          {"age80_up", 5}}, // lookup
      {1,2,3,4,5},  // valid_nums
      6};

  inline RuntimeEnum Vaxstatus = {
      {"none", "first", "full", "booster"},
      {{"none", 0}, {"first", 1}, {"full", 2}, {"booster", 3}},
      {0,1,2,3},  // valid_nums
      4};   // this should probably be moved into the vaxset struct?
}

// parameters used throughout the simulation that are better global
//    than passed into every function
namespace sim {
    inline int current_day = 0;
    
    inline int get_day() { 
        return current_day; 
    }
    
    inline void increment_day() { 
        ++current_day; 
    }
}

struct InfectParams {
  vector<float> sendrisk{};
  vector<float> recvrisk{};
  float basemultiplier{1.0};
  int immunehalflife {0};
};


struct InfectSet {
  vector<std::pair<string, InfectParams>> infectparams{};

  void print() {
    for (const auto& vp : infectparams) {
      fmt::print("variant: {:<9}\n  sendrisk={},\n  recvrisk={},\n  base={:.2f}, halflife={}\n",
                 vp.first,
                 vp.second.sendrisk,
                 vp.second.recvrisk,
                 vp.second.basemultiplier,
                 vp.second.immunehalflife);
    }
  }
};


struct Agetree {  // for 1 variant
  vector<absl::flat_hash_map<uint8_t,vector<vector<float>>>> tree{};  // would be matrices in Julia--must be vec of vec in c++

  void print() const {
    if (tree.empty()) {
      fmt::println("    Tree: <empty>");
      return;
    }

    for (size_t age_idx = 0; age_idx < tree.size(); age_idx++) {
      const auto& breakday_map = tree[age_idx];
      string age_name = Traits::Agegrp.to_str(age_idx + 1);
      fmt::println("    Age group: {}", age_name);

      if (breakday_map.empty()) {
        fmt::println("      <no breakdays>");
        continue;
      }

      // Sort breakdays for consistent output
      vector<uint8_t> breakdays;
      for (const auto& [day, _] : breakday_map) {
        breakdays.push_back(day);
      }
      std::sort(breakdays.begin(), breakdays.end());

      for (uint8_t day : breakdays) {
        const auto& condition_vec = breakday_map.at(day);
        fmt::println("      Day {}: {} conditions", day, condition_vec.size());

        for (size_t cond_idx = 0; cond_idx < condition_vec.size(); cond_idx++) {
          const auto& outcome_probs = condition_vec[cond_idx];
          string cond_name = Traits::Condition.to_str(cond_idx + 1); //(cond_idx < conditions.size()) ? conditions[cond_idx] : fmt::format("cond{}", cond_idx);

          fmt::print("        {}: [", cond_name);
          for (size_t i = 0; i < outcome_probs.size(); i++) {
            if (i > 0) fmt::print(", ");
            fmt::print("{:.2f}", outcome_probs[i]);
          }
          fmt::println("]");
        }
      }
    }
  }
};


struct ProgressionFactors {  // for one variant
  vector<float> riskadjust {};  // 0 elements, will be 6 elements if used
  absl::flat_hash_map<string, float>  // vaccine to single float value
      vaxhalflifeadjust {}; // might do uint8_t keys or RuntimeEnum keys or vector of whatever

  void print() const {
    fmt::println("  Progression Factors:");

    if (riskadjust.empty()) {
      fmt::println("    riskadjust: <empty>");
    } else {
      fmt::print("    riskadjust: [");
      for (size_t i = 0; i < riskadjust.size(); i++) {
        if (i > 0) fmt::print(", ");
        fmt::print("{:.2f}", riskadjust[i]);
      }
      fmt::println("]");
    }

    if (vaxhalflifeadjust.empty()) {
      fmt::println("    vaxhalflifeadjust: <empty>");
    } else {
      fmt::println("    vaxhalflifeadjust:");
      // Sort keys for consistent output
      vector<string> vax_names;
      for (const auto& [vax, _] : vaxhalflifeadjust) {
        vax_names.push_back(vax);
      }
      std::sort(vax_names.begin(), vax_names.end());

      for (const auto& vax : vax_names) {
        fmt::println("      {}: {:.2f}", vax, vaxhalflifeadjust.at(vax));
      }
    }
  }
};

struct Progression {     // for one variant
  Agetree tree {};  // index by variant index, string = variant name
  ProgressionFactors factors {};

  void print(const string& variant_name) const {
    fmt::println("\nVariant: {}", variant_name);
    factors.print();
    fmt::println("  Progression Tree:");
    tree.print();
  }
};

struct ProgressionSet {  // collection of all variants
  vector<Progression> progression{}; // index by variant uint8_t

  void print(const RuntimeEnum& variants) const {
    fmt::println("\n=== ProgressionSet ===");
    fmt::println("Total variants: {}", progression.size());

    for (size_t i = 0; i < progression.size(); i++) {
      string variant_name = (i < variants.names.size()) ? variants.names[i] : fmt::format("variant_{}", i);
      progression[i].print(variant_name);
    }
    fmt::println("\n=== End ProgressionSet ===\n");
  }
};

/*
access will look like
ProgressionSet progression{};  // assume it then gets loaded
progression[0].tree[0][5][0][0]  
    // we have 6 levels of qualifiers
    // 1) for variant "base" index = 0, 
    // 2) tree member, 
    // 3) agegrp "age0_19" index = 0, 
    // 4) breakday 5 key, 
    // 5) condition "nil" by index = 0,
    // 6) recovered probability by vector index for to recovered index = 0

*/

struct VaxParams {
  int reqdshots{1};
  int delay2ndshot{0}; // how to encode 'nothing'?
  int delaybooster{0}; // ditto
  int halflife{180}; // days to 50% decline in effectiveness
  int full_effect_days{15};
  float day1_effect{0.0};

  vector<std::pair<string, float>> infectfactor {}; // per variant reduction in infection
  vector<std::pair<string, vector<std::pair<string, float>>>> effectiveness {}; // by first, full, booster then variant=>infection reduction

  void print(const string& vax_name) const {
    fmt::println("  Vaccine: {}", vax_name);
    fmt::println("    Required shots: {}", reqdshots);
    fmt::println("    Delay 2nd shot: {} days", delay2ndshot);
    fmt::println("    Delay booster: {} days", delaybooster);
    fmt::println("    Half-life: {} days", halflife);
    fmt::println("    Full effect days: {}", full_effect_days);
    fmt::println("    Day 1 effect: {:.2f}", day1_effect);

    // Print infectfactor
    if (infectfactor.empty()) {
      fmt::println("    Infect factor: <empty>");
    } else {
      fmt::println("    Infect factor by variant:");
      for (const auto& [variant, factor] : infectfactor) {
        fmt::println("      {:<15}: {:.2f}", variant, factor);
      }
    }

    // Print effectiveness
    if (effectiveness.empty()) {
      fmt::println("    Effectiveness: <empty>");
    } else {
      fmt::println("    Effectiveness by shot type:");
      for (const auto& [shot_type, variant_effects] : effectiveness) {
        fmt::println("      {}:", shot_type);
        for (const auto& [variant, effect] : variant_effects) {
          fmt::println("        {:<15}: {:.2f}", variant, effect);
        }
      }
    }
  }
};


struct VaxSet {
  vector<std::pair<string, VaxParams>> vaxset{};
  RuntimeEnum shot_types = {{"first", "full", "booster"},           // names
                            {{"first", 0}, {"full", 1}, {"booster", 2}},  // lookup
                            {0,1,2},  // valid_nums
                            3}; // nextnum --  I guess this will be hard-coded

  void print() const {
    fmt::println("\n=== VaxSet ===");
    fmt::println("Total vaccines: {}", vaxset.size());

    fmt::print("Shot types: ");
    for (size_t i = 0; i < shot_types.names.size(); i++) {
      if (i > 0) fmt::print(", ");
      fmt::print("{}", shot_types.names[i]);
    }
    fmt::println("\n");

    for (const auto& [vax_name, vax_params] : vaxset) {
      vax_params.print(vax_name);
      fmt::println("");
    }

    fmt::println("=== End VaxSet ===\n");
  }
};

//
// vaccination schedules
//
struct PerVaxSpec {
  string vax_name{};
  float mix{};
  int starting_doses{};
  float pct2ndshot {};
  float pctboost {};
  vector<string> alternate {};

  void print() const {
    fmt::println("    Vaccine: {}", vax_name);
    fmt::println("      Mix: {:.2f}", mix);
    fmt::println("      Starting doses: {}", starting_doses);
    fmt::println("      Pct 2nd shot: {:.2f}", pct2ndshot);
    fmt::println("      Pct boost: {:.2f}", pctboost);
    if (alternate.empty()) {
      fmt::println("      Alternates: <none>");
    } else {
      fmt::print("      Alternates: ");
      for (size_t i = 0; i < alternate.size(); i++) {
        if (i > 0) fmt::print(", ");
        fmt::print("{}", alternate[i]);
      }
      fmt::println("");
    }
  }
};

struct VaxSched {
  vector<PerVaxSpec> vaxesincluded {};
  std::pair<int, int> dayrange{};
  float targetpct {};
  vector<string> filtervec {};
  string shotmode {};
  vector<float> pattern {};
  string spreadfunc {};  // will change to function pointer later

  void print() const {
    fmt::println("\n=== VaxSched ===");

    // Print vaccines included
    fmt::println("Vaccines included: {}", vaxesincluded.size());
    for (const auto& vax_spec : vaxesincluded) {
      vax_spec.print();
    }

    // Print day range
    fmt::println("\n  Day range: [{}, {}]", dayrange.first, dayrange.second);

    // Print target percentage
    fmt::println("  Target pct: {:.2f}", targetpct);

    // Print filter vector
    if (filtervec.empty()) {
      fmt::println("  Filter: <none>");
    } else {
      fmt::print("  Filter: ");
      for (size_t i = 0; i < filtervec.size(); i++) {
        if (i > 0) fmt::print(", ");
        fmt::print("{}", filtervec[i]);
      }
      fmt::println("");
    }

    // Print shot mode
    fmt::println("  Shot mode: {}", shotmode);

    // Print pattern
    if (pattern.empty()) {
      fmt::println("  Pattern: <empty>");
    } else {
      fmt::print("  Pattern: [");
      for (size_t i = 0; i < pattern.size(); i++) {
        if (i > 0) fmt::print(", ");
        fmt::print("{:.2f}", pattern[i]);
      }
      fmt::println("]");
    }

    // Print spread function
    fmt::println("  Spread func: {}", spreadfunc.empty() ? "null" : spreadfunc);

    fmt::println("=== End VaxSched ===\n");
  }
};


//
// social params
//
struct SocialParams {
  // Scalar parameters (loaded from JSON)
  float gammashape {0.0};
  float indoor_uplift {1.0};

  // Matrix data (loaded from JSON)
  array<array<float, 5>, 4> contactfactors {};  // 4 rows (contact_rows) × 5 cols (age groups)
  array<array<float, 5>, 6> touchfactors {};    // 6 rows (touch_rows) × 5 cols (age groups)

  // Row and column labels (const metadata, initialized in constructor)
  const vector<string> touch_rows {};
  const vector<string> contact_rows {};
  const vector<string> age_columns {};

  // Default constructor initializes the const label vectors
  SocialParams()
      : touch_rows{"unexposed", "recovered", "nil", "mild", "sick", "severe"},
        contact_rows{"nil", "mild", "sick", "severe"},
        age_columns{"age0_19", "age20_39", "age40_59", "age60_79", "age80_up"} {
  }

  void print() {
    fmt::print("gammashape: {} indoor_uplift: {}\n", gammashape, indoor_uplift);

    // Print contactfactors with row and column labels
    fmt::print("contactfactors ({}x{}):\n", contact_rows.size(), age_columns.size());

    fmt::print("{:<10}", "");
    for (const auto& colhdr : age_columns) {
      fmt::print("{:>11}", colhdr);
    }
    fmt::print("\n");

    for (size_t i = 0; i < contactfactors.size(); ++i) {
      fmt::print("{:>10} ", contact_rows[i]);
      for (size_t j = 0; j < contactfactors[i].size(); ++j) {
        fmt::print("{:>10.2f} ", contactfactors[i][j]);
      }
      fmt::print("\n");
    }
    fmt::print("\n");

    // print touchfactors with row and column labels
    fmt::print("touchfactors ({}x{}):\n", touch_rows.size(), age_columns.size());

    fmt::print("{:<10}", "");
    for (const auto& colhdr : age_columns) {
      fmt::print("{:>11}", colhdr);
    }
    fmt::print("\n");

    for (size_t i = 0; i < touchfactors.size(); ++i) {
      fmt::print("{:>10} ", touch_rows[i]);
      for (size_t j = 0; j < touchfactors[i].size(); ++j) {
        fmt::print("{:>10.2f} ", touchfactors[i][j]);
      }
      fmt::print("\n");
    }
    fmt::print("\n");
  }
};


//
// container for all of the parameters required for a model
//
struct ModelParams {

  // Traits for each person in popdata

  GeoData geodata;
  RuntimeEnum Condition;
  RuntimeEnum Status;
  RuntimeEnum Agegrp;

  //based on variants parameters json file
  RuntimeEnum variants;
  InfectSet infectset;
  ProgressionSet progressionset;
  vector<float> trvec;

  SocialParams socialdata;  // Changed from json to SocialParams
  VaxSet vaxset;
  RuntimeEnum vaxlist;
  VaxSched vaxsched;  // Changed from json to VaxSched
};

// here we define all of the containers for the model parameters, load them,
// and for convenience pass them into the model building and running code as
// one container.
// the file is organized by container type followed by struct model_params that
// puts them into one container.


// free function declarations for loading the structs from json file inputs

json load_json_params(string fpath); 


GeoData load_geodata_csv(const std::string& filename);


std::tuple<RuntimeEnum, InfectSet> load_variants_data(json jdata);


std::tuple<ProgressionSet, vector<float>> load_progression_set(json jdata);


std::tuple<InfectSet, ProgressionSet, vector<float>, RuntimeEnum> load_infect_params(string fpath);


std::tuple<VaxSet, RuntimeEnum> load_vax_data(string fpath, RuntimeEnum variants);


VaxSched load_vax_sched(const string &fname, RuntimeEnum vaxlist);


SocialParams load_social_params(string social_path);
