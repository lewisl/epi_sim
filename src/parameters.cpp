#include <csv2/reader.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
// #include <iomanip>
#include <cstdlib>
#include <filesystem>
#include <unordered_map>
#include <tuple>
#include <utility>
// #include <yaml-cpp/node/parse.h>
// #include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
// #include <fmt/base.h>
#include <fmt/format.h>  // only get what I use: about 12k in the executable!
#include <fmt/ranges.h>  // for printing containers like vector

using json = nlohmann::json;
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
  std::unordered_map<std::string, int> lookup; // Name-to-Index (String -> Number)
  int nextnum{0};

  void add_item(std::string newname) {
      if (lookup.find(newname) == lookup.end()) { // Avoid duplicates
          names.push_back(newname);
          lookup[newname] = nextnum;
          nextnum++;
      }
    }

  // Num -> String (Instant)
  std::string to_string(int i) {
    return (i >= 0 && i < names.size()) ? names[i] : "INVALID";
  }

    // functor that returns the int corresponding to the string
    int operator()(const string& name) const {
      auto it = lookup.find(name);
      return (it != lookup.end()) ? it->second : -1;
    }
  
  void print() {
    for (size_t i = 0; i < names.size(); ++i) {
      fmt::print("{:>2}: {:<8}\n", i, names[i]);}
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

std::tuple<RuntimeEnum, InfectSet> load_variants_data(string fpath) {
  json vdata = load_json_params(fpath);

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

  return {variants, infectset};  // gonna have 3 more real data structures
}

/*
vector indexed by agegrp int of
  vector indexed by breakday of
     vector<vector<float> a matrix of progression probabilities

need a helper function that builds a progression probabilty matrix from json input
*/
struct Agetree {
  vector<vector<vector<vector<float>>>> tree {};
};

struct ProgressionFactors {
  vector<float> riskadjust;
  std::unordered_map<string, float>
      vaxhalflifeadjust; // might do uint8_t keys or RuntimeEnum keys or vector of whatever
};

struct ProgressionParams {
  Agetree tree;
  ProgressionFactors factors;
};

/*
struct progressionset

variant => ProgressionParams



*/


// struct trvec?

// struct variants  -- done



//
// vaccine data
//


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
    cout << "\n";

    for (size_t i = 0; i < contactfactors.size(); ++i) {
      fmt::print("{:>10} ", contact_rows[i]);
      for (size_t j = 0; j < contactfactors[i].size(); ++j) {
        fmt::print("{:>10.2f} ", contactfactors[i][j]);
      }
      cout << "\n";
    }
    cout << "\n";

    // print touchfactors with row and column labels
    fmt::print("touchfactors ({}x{}):\n", touch_rows.size(), age_columns.size());

    fmt::print("{:<10}", "");
    for (const auto& colhdr : age_columns) {
      fmt::print("{:>11}", colhdr);
    }
    cout << "\n";

    for (size_t i = 0; i < touchfactors.size(); ++i) {
      fmt::print("{:>10} ", touch_rows[i]);
      for (size_t j = 0; j < touchfactors[i].size(); ++j) {
        fmt::print("{:>10.2f} ", touchfactors[i][j]);
      }
      cout << "\n";
    }
    cout << "\n";
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

  vector<float> trvec;
  // ProgressionStruct progression;
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
  auto [variants, infectset] = load_variants_data(variants_path);

  // test the functor
  cout << "test functor with variant \"base\" " << variants("base") << "\n";

  
  // Use aggregate initialization to construct model_params with all members at once
  // This avoids assignment to socialdata (which has const members)
  // note the curly braces: this is initialization, NOT a call to the default constructor
  return ModelParams{
    .geodata = std::move(geodata),
    .variants = std::move(variants),
    .infectset = std::move(infectset),
    // .variantdata = load_json_params(variants_path),
    .socialdata = load_social_params(social_path),
    .vaccinesdata = load_json_params(vaccines_path),
    .vaxsched = load_json_params(vaxsched_path)
  };
}
