#pragma once

#include "lib_includes.h"

#include "traits.h"
#include "ring_traits.h"
#include "helpers.h"    // for shifter
#include <functional>
#include "cases.h"

// using json = nlohmann::json; // for not ordered mapping
using json = nlohmann::ordered_json;
using std::array;
using std::string;
using std::vector;


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
      fmt::print("{:<4}{:<8}{:<18}{:<18}{:<8}{:<10}{:<10}{:<10}{:<12}{:<12}{:<12}\n",
                "", "fips", "county", "city", "state",
                "sizecat", "pop", "density", "anchor", "indoor_st", "indoor_end");

      // Rows
      for (size_t i = 0; i < num_rows; ++i) {
          fmt::print("{:>2}: {:<8}{:<18}{:<18}{:<8}{:<10}{:<10}{:<10.3}{:<12}{:<12}{:<12}\n",
              //  fmt::format("{}:", i),
              i,
                fips[i],
                county[i],
                city[i],
                state[i],
                sizecat[i],
                pop[i],
                density[i],
                anchor[i],
                indoor_st[i],
                indoor_end[i]);
      }
    }    
};



struct InfectParams {
  vector<float> sendrisk{};
  vector<float> recvrisk{};
  vector<float> recovery_immunity{};
  float basemultiplier{1.0};
  int immunehalflife {0};
};



struct ProgressionFactors {  // for one variant
  vector<float> riskadjust {};  // 0 elements, will be 6 elements if used
  absl::flat_hash_map<string, float>  // vaccine to single float value
      vaxhalflifeadjust {}; // might do uint8_t keys or MapEnum keys or vector of whatever

  void print() const {
    fmt::println("  Progression Factors:");

    if (riskadjust.empty()) {
      fmt::println("    riskadjust: <empty>");
    } else {
      fmt::print("    riskadjust: [");
      for (size_t i = 0; i < riskadjust.size(); ++i) {
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

using Agetree = vector<absl::flat_hash_map<uint8_t,vector<vector<float>>>>;

struct Progression { // for one variant
  vector<absl::flat_hash_map<uint8_t,vector<vector<float>>>> tree {};
  // Agetree tree {};  // index by variant index, string = variant name
  ProgressionFactors factors {};

  void print(std::string variant_name) const {
    fmt::println("\nVariant: {}", variant_name);
    factors.print();
    fmt::println("  Progression Tree:");
    tree_print();
  }

    void tree_print() const {
    if (tree.empty()) {
      fmt::println("    Tree: <empty>");
      return;
    }

    for (size_t age_idx = 0; age_idx < tree.size(); ++age_idx) {
      const auto& breakday_map = tree[age_idx];
      std::string age_name = Agegrp::names[age_idx+1UZ];
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

        for (size_t cond_idx = 0; cond_idx < condition_vec.size(); ++cond_idx) {
          const auto &outcome_probs = condition_vec[cond_idx];
          std::string cond_name = Condition::names[cond_idx + 1];
          // string cond_name = Trait::Condition.to_str(cond_idx + 1); //(cond_idx < conditions.size()) ? conditions[cond_idx] : fmt::format("cond{}", cond_idx);

          fmt::print("        {}: [", cond_name);
          for (size_t i = 0; i < outcome_probs.size(); ++i) {
            if (i > 0) fmt::print(", ");
            fmt::print("{:.2f}", outcome_probs[i]);
          }
          fmt::println("]");
        }
      }
    }
  }

};

struct ProgressionSet {  // collection of all variants
  vector<Progression> progression{}; // index by variant using values of type uint8_t

  void print(const vector<Variant>& variants) const {
    fmt::println("\n=== ProgressionSet ===");
    fmt::println("Total variants: {}", progression.size());

    for (size_t i = 0; i < progression.size(); ++i) {
      std::string variant_name = variants[i].show();
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
  vector<VaxParams> params{};

  const VaxParams& at(Vax vax) const {
    const size_t vax_idx = idx(vax);
    if (vax_idx == 0 || vax_idx >= params.size()) {
      throw std::runtime_error("Invalid vaccine lookup for VaxSet");
    }
    return params[vax_idx];
  }

  [[nodiscard]] size_t size() const {
    return params.empty() ? 0 : params.size() - 1;
  }

  void print() const {
    fmt::println("\n=== VaxSet ===");
    fmt::println("Total vaccines: {}", size());

    fmt::print("Shot types: ");
    for (size_t i = 1; i < Vaxstatus::names.size(); ++i) {
      if (i > 1) fmt::print(", ");
      fmt::print("{}", Vaxstatus::names[i]);
    }
    fmt::println("\n");

    for (size_t i = 1; i < params.size(); ++i) {
      params[i].print(Vax::names[i]);
      fmt::println("");
    }

    fmt::println("=== End VaxSet ===\n");
  }
};

//
// vaccination schedules
//
struct PerVaxSpec {
  Vax vax{};
  float mix{};
  int starting_doses{};
  int doses{};
  float pct2ndshot {};
  float pctboost {};
  vector<Vax> alternate {};

  void print() const {
    fmt::println("    Vaccine: {}", vax.show());
    fmt::println("      Mix: {:.2f}", mix);
    fmt::println("      Starting doses: {}", starting_doses);
    fmt::println("      Doses remaining: {}", doses);
    fmt::println("      Pct 2nd shot: {:.2f}", pct2ndshot);
    fmt::println("      Pct boost: {:.2f}", pctboost);
    if (alternate.empty()) {
      fmt::println("      Alternates: <none>");
    } else {
      fmt::print("      Alternates: ");
      for (size_t i = 0; i < alternate.size(); ++i) {
        if (i > 0) fmt::print(", ");
        fmt::print("{}", alternate[i].show());
      }
      fmt::println("");
    }
  }
};



struct VaxSched {
  vector<PerVaxSpec>         vaxesincluded {};
  std::pair<int, int>        dayrange {};
  float                      targetpct {};
  vector<Agegrp>             filtervec {};   // was vector<string>
  string                     shotmode {};
  vector<float>              pattern {};
  std::function<float(int)>  spreadfunc {};

  void init_spreadfunc() {
    int   schedlength = dayrange.second - dayrange.first + 1;
    float scale       = (float)(pattern.size() - 1) / (float)schedlength;

    spreadfunc = [dayrange  = this->dayrange,
                  targetpct = this->targetpct,
                  pattern   = this->pattern,
                  scale](int day) -> float {

      float p = (float)(day - dayrange.first + 1) * scale;
      p = std::min(p, (float)(pattern.size() - 1));

      int   lo   = (int)p;
      int   hi   = std::min(lo + 1, (int)pattern.size() - 1);
      float frac = p - (float)lo;

      return (pattern[lo] + frac * (pattern[hi] - pattern[lo])) * scale * targetpct;
    };
  }

  void reset_doses() {
    for (auto& spec : vaxesincluded)
      spec.doses = spec.starting_doses;  // PROBLEM:  no member doeses in vaxesincluded whivch is probably just a vector of vax nanmes
  }


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
      for (size_t i = 0; i < filtervec.size(); ++i) {
        if (i > 0) fmt::print(", ");
        fmt::print("{}", filtervec[i].show());
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
      for (size_t i = 0; i < pattern.size(); ++i) {
        if (i > 0) fmt::print(", ");
        fmt::print("{:.2f}", pattern[i]);
      }
      fmt::println("]");
    }

    // Print spread function
    // fmt::println("  Spread func: {}", spreadfunc.empty() ? "null" : spreadfunc);

    fmt::println("=== End VaxSched ===\n");
  }
};

struct VaxSchedSet {
  vector<std::pair<string, VaxSched>> schedules {};

  [[nodiscard]] bool empty() const { return schedules.empty(); }
  [[nodiscard]] size_t size() const { return schedules.size(); }

  void reset_doses() {
    for (auto& [name, sched] : schedules)
      sched.reset_doses();
  }

  void print() const {
    fmt::println("\n=== VaxSchedSet ===");
    fmt::println("Schedules loaded: {}", schedules.size());
    for (const auto& [name, sched] : schedules) {
      fmt::println("Schedule: {}", name);
      sched.print();
    }
    fmt::println("=== End VaxSchedSet ===\n");
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

  // Trait for each person in popdata

  GeoData geodata;

  //based on variants parameters json file
  vector<Variant> variants;
  vector<InfectParams> infectparams;
  ProgressionSet progressionset;
  array<float, 6> trvec;

  SocialParams socialdata;  // Changed from json to SocialParams
  VaxSet vaxset;
  VaxSchedSet vaxschedset;
  RingTraits ringtraits;
};

// here we define all of the containers for the model parameters, load them,
// and for convenience pass them into the model building and running code as
// one container.
// the file is organized by container type followed by struct model_params that
// puts them into one container.


// free function declarations for loading the structs from json file inputs

json load_json_params(string fpath); 


GeoData load_geodata_csv(const std::string& filename);


std::tuple<vector<Variant>, vector<InfectParams>> load_variants_data(json jdata);


std::tuple<ProgressionSet, array<float, 6>> load_progression_set(json jdata);


std::tuple<vector<InfectParams>, ProgressionSet, array<float, 6>, vector<Variant>> load_infect_params(string fpath);


VaxSet load_vax_data(string fpath);

VaxSched load_vax_sched(const string &fname);

VaxSchedSet load_vax_sched_set(const string &dirpath);


SocialParams load_social_params(string social_path);

RingTraits load_ring_traits(string fpath);

// Helper function to print infectparams
void print_infectparams(const vector<InfectParams>& infectparams, const vector<Variant>& variants);
