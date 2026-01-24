#include <csv2/reader.hpp>  // adequate...
#include <string>
#include <algorithm>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
// #include <iomanip>
#include <cstdlib>
#include <filesystem>
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
#include "categories.h"

// using json = nlohmann::json;
using json = nlohmann::ordered_json; 
using std::cout;
using std::string;
using std::vector;
using std::array;
namespace fs = std::filesystem;

// forward declarations
inline void shifter(vector<float> &arr, const float newmin, const float newmax);

// here we define all of the containers for the model parameters, load them,
// and for convenience pass them into the model building and running code as
// one container.
// the file is organized by container type followed by struct model_params that
// puts them into one container.


// for testing only
// Keep all paths as plain strings (avoid std::filesystem::path conversions).

const fs::path project_dir = fs::path(std::getenv("HOME")) / "code" / "epi_sim";
const fs::path param_dir = "sample_parameters";
const fs::path vax_sched_dir = "vaccine_100k";
const fs::path variants_fname = "variants.json";
const fs::path geodata_fname = "geo2data.csv";
const fs::path social_fname = "socialparams.json";
const fs::path vaccines_fname = "vaccines.json";
const fs::path vax_sched_fname = "loc38015_old.json";

// Full paths
const string variants_path = (project_dir / param_dir / variants_fname).string();
const string geodata_path = (project_dir / param_dir / geodata_fname).string();
const string social_path = (project_dir / param_dir / social_fname).string();
const string vaccines_path = (project_dir / param_dir / vaccines_fname).string();
const string vax_sched_path = (project_dir / param_dir / vax_sched_dir / vax_sched_fname).string();


struct RuntimeEnum {
  std::vector<std::string> names; // Index-to-Name (Number -> String)
  absl::flat_hash_map<std::string, int> lookup; // Name-to-Index (String -> Number)
  int nextnum{0};

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
      return (it != names.end()) ? std::distance(names.begin(), it) : -1;
  }

  // String → Index (hash map - for large N or explicit use)
  int to_int(const std::string& name) const {
      auto it = lookup.find(name);
      return (it != lookup.end()) ? it->second : -1;
  }
  
  // Num -> String (Instant)
  std::string to_string(int i) {
    return (i >= 0 && i < names.size()) ? names[i] : "INVALID";
  }
  
  void print() const {
    for (size_t i = 0; i < names.size(); ++i) {
      fmt::print("{:>2}: {:<8}\n", i, names[i]);
    }
  }
};


json load_json_params(string fpath) {

  try {
    std::ifstream fcontent(fpath);
    json data = json::parse(fcontent);

    return data;
  }

  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
      return json();  // empty object
  }
}

//
// geodata: this is done
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

GeoData load_geodata_csv(const std::string& filename) {
  using namespace csv2;

  GeoData data;
    
  Reader<delimiter<','>,       // this is a templated class instance called csv
    quote_character<'"'>, 
    first_row_is_header<true>,
    trim_policy::trim_whitespace> csv;
  
  if (!csv.mmap(filename)) {
      throw std::runtime_error("Failed to open CSV file: " + filename);
  }

  // Read all rows
  std::vector<std::string> values; // hold cells per row
  values.reserve(20);  // saves allocations for most elements per row cases
  for (const auto& row : csv) {
    values.clear();   
    for (const auto& cell : row) {
        std::string value;
        cell.read_value(value);
        values.push_back(value);
    }
      
    // push into typed columns (order matches CSV)
    data.fips.push_back(std::stoi(values[0]));
    data.county.push_back(values[1]);
    data.city.push_back(values[2]);
    data.state.push_back(values[3]);
    data.sizecat.push_back(std::stoi(values[4]));
    data.pop.push_back(std::stoi(values[5]));
    data.density.push_back(std::stof(values[6]));
    data.anchor.push_back(values[7]);
    data.indoor_st.push_back(values[8]);
    data.indoor_end.push_back(values[9]);
      
    data.num_rows++;
  }

  shifter(data.density, 0.9, 1.25);  // turn population density of cities into a compressed index
  return data;
}


//
// variant data supplies infectset, progressionset, trvec, variants
//


void print_variants_data(json v) {
    cout << "\n\n================\nexamining variants data"<< ", count of top-level nodes: " << v.size() << "\n";    
    cout << v.dump(2) << "\n====================\n";

    // let's see what or how we can select parts
    // for (auto element : v.items()) {  // items() required for iteration of key, value
    //   cout << "==== " << element.key() << " ====\n";
    //   cout << v[element.key()]["spread"]["sendrisk"] << "\n";
    //   cout << "===================================\n";
    // }
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

std::tuple<RuntimeEnum, InfectSet> load_variants_data(json vdata) {
  // json vdata = load_json_params(fpath);

  RuntimeEnum variants{};
  for (auto variant : vdata.items()) {
    variants.add_item(variant.key());
  }

  InfectSet infectset{};
  for (auto variant : vdata.items()) {
    infectset.infectparams.emplace_back( // pair members string, InfectParams
        variant.key(), InfectParams{
                           .sendrisk = variant.value()["spread"]["sendrisk"],
                           .recvrisk = variant.value()["spread"]["recvrisk"],
                           .basemultiplier = variant.value()["spread"]["basemultiplier"],
                           .immunehalflife = variant.value()["immunity"]["immunehalflife"]});
  }

  return {variants, infectset};  // gonna have more data structures: ProgressionParams, trvec
}

/*
vector indexed by agegrp int of
  vector indexed by breakday of
     vector<vector<float> a matrix of progression probabilities

to parse: 
"<variant>" 
      "progression_tree"
          "<agegrp>"
              "<breakday>"
                  4 sickness conditions: "nil", "mild", "sick", "severe"
                  for each of vector<float> in [0.0, 1.0]: [recover, nil, mild, sick, severe, dead]

if no tree apply riskadjust and vaxhalflifeadjust to base
*/

struct Agetree {  // for 1 variant
  vector<absl::flat_hash_map<uint8_t,vector<vector<float>>>> tree{};  // would be matrices in Julia--must be vec of vec in c++
  //age->vector indices 0..4/breakday->flat_hash_map<sparse ints, vector<condition indices 0..3><vector<float>> 6 values summing to 1.0

  void print() const {
    if (tree.empty()) {
      fmt::println("    Tree: <empty>");
      return;
    }

    for (size_t age_idx = 0; age_idx < tree.size(); age_idx++) {
      const auto& breakday_map = tree[age_idx];
      string age_name = Agegrp::to_str(age_idx + 1);
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
          string cond_name = Condition::to_str(cond_idx + 1); //(cond_idx < conditions.size()) ? conditions[cond_idx] : fmt::format("cond{}", cond_idx);

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


std::tuple<ProgressionSet, vector<float>> load_progression_set(json vdata) {
  // json vdata = load_json_params(fpath);
  ProgressionSet progressionset{};

  for (const auto &[variant, body] : vdata.items()) {  // variant loop
    ProgressionFactors factors{};

    auto jsontree = body["progression_tree"];
    auto jsonfactors = body["progression_factors"];

    // create members of ProgressionFactors factors
    factors.riskadjust = jsonfactors["riskadjust"].get<vector<float>>();
    for (const auto &[vax, num] : jsonfactors["vaxhalflifeadjust"].get<absl::flat_hash_map<string, float>>()) {
            factors.vaxhalflifeadjust[vax] = num;
    };

    // create members of Agetree tree
    vector<absl::flat_hash_map<uint8_t, vector<vector<float>>>> age_vec{};
    for (const auto& [age, body_age] : jsontree.items()) {  // age is a string we won't store because it will be the vector index
      absl::flat_hash_map<uint8_t, vector<vector<float>>> one_age_map {};
      for (const auto& [duration, body_duration] : body_age.items()) {
        vector<vector<float>> tmpvec{};
        for (const auto &[cond, probvec] : body_duration.items()) {  // cond is a string won't be stored because it will be the vect idx
          tmpvec.push_back(probvec);
        }
        one_age_map[std::stoi(duration)] = tmpvec;
      };
      age_vec.push_back(one_age_map);
    };
    Progression pg {
      .tree = Agetree{age_vec},
      .factors = factors
      };

    progressionset.progression.push_back(pg);

  } // variant loop

  // pre-allocate small vector for performance in progression kernel function
  vector<float> trvec = vector<float>(6, 0.0f); 
  
  return {progressionset, trvec};
}

std::tuple<InfectSet, ProgressionSet, vector<float>, RuntimeEnum> load_infect_params(string fpath) {
  // use one big json file for multiple output structs, etc.
  json vdata = load_json_params(fpath);

  auto [variants, infectset] = load_variants_data(vdata);

  auto [progressionset, trvec] = load_progression_set(vdata);


  return {infectset, progressionset, trvec, variants};
};

/*
access will look like
ProgressionSet progression{};  // assume it then gets loaded
progression[0].tree[0][5][0][0]  
    // we have 6 levels of qualifiers
    // 1) for variant "base" index, 
    // 2) tree member, 
    // 3) agegrp "age0_19" index, 
    // 4) breakday 5 key, 
    // 5) condition "nil" by index,
    // 6) recovered probability by vector index

*/
// struct trvec?

// struct variants  -- done



//
// vaccine data
//

struct VaxParams {
  int reqdshots{};
  int delay2ndshot{}; // how to encode 'nothing'?
  int delaybooster{}; // ditto
  int halflife{}; // days to 50% decline in effectiveness
  int full_effect_days{};
  float day1_effect{};

  vector<std::pair<uint8_t, float>>
      infectfactor; // per variant reduction in infection
  vector<std::pair<uint8_t, vector<std::pair<uint8_t, float>>>> effectiveness; // by first, full, booster then variant=>infection reduction
};


struct VaxSet {
  vector<std::pair<string, VaxParams>> vaxset{};

  void print() {
    for (const auto& vp : vaxset) {
      fmt::print("vaccine: {:<9}\n  sendrisk={},\n  recvrisk={},\n  base={:.2f}, halflife={}\n", 1,2,3,4.2, 5);
                //  vp.first,
                //  vp.second.sendrisk,
                //  vp.second.recvrisk,
                //  vp.second.basemultiplier,
                //  vp.second.immunehalflife);
    }
  }
};

void print_vaccines_data(json v) {
  string subkey = "effectiveness";
  
    cout << "\n\n================\nexamining vaccines data"<< ", count of top-level nodes: " << v.size() << "\n";    

    // let's see what or how we can select parts
    for (auto element : v.items()) {    // items() required for iteration of key, value
        cout << "==== " << element.key() << " " << subkey << " ====\n";
        cout << v[element.key()][subkey].dump(2) << "\n";
        cout << "===================================\n";
      }
}


//
// vaccination schedules
//
// printed with json dump in test.cpp


//
// social params - this is done
//

struct socialparams {
  // Scalar parameters (loaded from JSON)
  float gammashape {};
  float indoor_uplift {};

  // Matrix data (loaded from JSON)
  array<array<float, 5>, 4> contactfactors {};  // 4 rows (contact_rows) × 5 cols (age groups)
  array<array<float, 5>, 6> touchfactors {};    // 6 rows (touch_rows) × 5 cols (age groups)

  // Row and column labels (const metadata, initialized in constructor)
  const vector<string> touch_rows;
  const vector<string> contact_rows;
  const vector<string> age_columns;

  // Default constructor initializes the const label vectors
  socialparams()
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

socialparams load_social_params(string social_path) {
  json data = load_json_params(social_path);

  socialparams socialp;
  socialp.gammashape = data["gammashape"];
  socialp.indoor_uplift = data["indoor_uplift"];

  // build touchfactors by rows (using struct's const members)
  for (size_t rowidx = 0; rowidx < socialp.touch_rows.size(); ++rowidx) {
    for (size_t colidx = 0; colidx < socialp.age_columns.size(); ++colidx) {
      socialp.touchfactors[rowidx][colidx] = data["touchfactors"][socialp.age_columns[colidx]][socialp.touch_rows[rowidx]]; // effectively transposing the json
    }
  }

  // build contactfactors by rows (using struct's const members)
  for (size_t rowidx = 0; rowidx < socialp.contact_rows.size(); ++rowidx) {
    for (size_t colidx = 0; colidx < socialp.age_columns.size(); ++colidx) {
      socialp.contactfactors[rowidx][colidx] = data["contactfactors"][socialp.age_columns[colidx]][socialp.contact_rows[rowidx]]; // effectively transposing the json
    }
  }

  return socialp;
}


//
// helpers    should move to helpers .h or .cpp
//
inline void shifter(vector<float> &arr, const float newmin, const float newmax) {  

  auto minposition = std::min_element(arr.begin(), arr.end());
  auto maxposition = std::max_element(arr.begin(), arr.end());
  const float oldmin = *minposition;
  const float oldmax = *maxposition;

  // Handle edge case: all values are identical
  if (oldmax == oldmin) {
    std::fill(arr.begin(), arr.end(), 1.0f);  // Neutral multiplier
  } else {
    for (auto &element : arr) {
      element = newmin + (newmax - newmin) / (oldmax - oldmin) * (element - oldmin); // we can put this in a function if needed
    }
  }
}


//
// container for all of the parameters required for a model
//
struct ModelParams {
  GeoData geodata;

  //based on variants parameters json file
  RuntimeEnum variants;
  InfectSet infectset;
  ProgressionSet progressionset;
  vector<float> trvec;
  
  socialparams socialdata;  // Changed from json to socialparams
  json vaccinesdata;
  json vaxsched;  // this will need to be loades separately--not part of building model
};

// free function for factory pattern
ModelParams load_model_params(string geo_path, string variants_path,
    string social_path, string vaxsched_path) {

  // first build each need datastructure;
  //          then wrap all of them in the aggregate initialization of the container
  GeoData geodata = load_geodata_csv(geo_path);

  auto [infectset, progressionset, trvec, variants] = load_infect_params(variants_path);

  
  // Use aggregate initialization to construct model_params with all members at once
  // This avoids assignment to socialdata (which has const members)
  // note the curly braces: this is initialization, NOT a call to the default constructor
  return ModelParams{
    .geodata = std::move(geodata),
    .variants = std::move(variants),
    .infectset = std::move(infectset),
    .progressionset = std::move(progressionset),
    .trvec = std::move(trvec),
    .socialdata = load_social_params(social_path),
    .vaccinesdata = load_json_params(vaccines_path),
    .vaxsched = load_json_params(vaxsched_path),
  };
}
