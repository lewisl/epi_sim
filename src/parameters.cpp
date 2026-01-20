#include <csv2/reader.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <filesystem>
#include <ranges>
// #include <yaml-cpp/node/parse.h>
// #include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <fmt/base.h>
// #include <fmt/format.h>

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


void print_geodata(const GeoData& data) {
    fmt::print("Loaded {} rows with {} columns:\n\n", 
               data.num_rows, data.column_names.size());

    // Header
    fmt::print(fmt::runtime("{:<4}{:<8}{:<18}{:<18}{:<8}{:<10}{:<10}{:<10}{:<12}\n"),
               "", "fips", "county", "city", "state", 
               "sizecat", "pop", "density", "anchor");
    
    // Rows
    for (size_t i = 0; i < data.num_rows; ++i) {
        fmt::print(fmt::runtime("{:>2}: {:<8}{:<18}{:<18}{:<8}{:<10}{:<10}{:<10.3}{:<12}\n"),
            //  fmt::format("{}:", i),
            i,
              data.fips[i],
              data.county[i],
              data.city[i],
              data.state[i],
              data.sizecat[i],
              data.pop[i],
              data.density[i],
              data.anchor[i]);
    }
}

//
// variant data supplies infectset, progressionset, trvec, variantlist
//

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

//
// infectset
//
// for each variant: immunity
//                      immunehalflife
//                      recovery_immunity per variant
//                    progression factors
//                      riskadjust: vector<float, 6> per condition
//                      vaxhalflifeadjust, float between 0.0 and 1.0 per vaccine
//                    progression_tree:  big tree or use factors:   which
//                               factors?
//                    spread: dict
//                      basemultiplier: float
//                      recvrisk: vector<float, 5> values per agegrp
//                      sendrisk: vector<float, 26> float between 0.0 and 2.0 for each day being sick





//
// vaccine data
//

struct NumNamePair {
  vector<string> name;
  vector<int> num;
  int nextnum{0};

  void add_item(string newname, int newnum) {
    name.push_back(newname);
    num.push_back(newnum);
    nextnum++;
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
// social params
//



/*
gammashape: float
indoor_uplift: float
social.contactfactors matrix 4 x 5 float conditions x agegrp
        "age0_19": {
        "nil": 1.1,
        "mild": 1.1,
        "sick": 0.7,
        "severe": 0.5
        },
social.touchfactors 6 x 5 matrix float 2 statuses 4 conditions by agegrp
            "unexposed": 0.55,
            "recovered": 0.55,
            "nil": 0.55,
            "mild": 0.55,
            "sick": 0.28,
            "severe": 0.18
*/

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
      age_columns{"age0_19", "age20_39", "age40_59", "age60_79", "age80_up"}
  {}
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

// move into struct when working...
void print_social_struct(socialparams social) {
  cout << "gammashape: " << social.gammashape << "\n";
  cout << "indoor_uplift: " << social.indoor_uplift << "\n\n";

  // Print contactfactors with row and column labels
  cout << "contactfactors (" << social.contact_rows.size() << "x" << social.age_columns.size() << "):\n";
  cout << "           ";
  for (const auto& col : social.age_columns) {
    cout << std::setw(10) << col << " ";
  }
  cout << "\n";

  for (size_t i = 0; i < social.contactfactors.size(); ++i) {
    cout << std::setw(10) << social.contact_rows[i] << " ";
    for (size_t j = 0; j < social.contactfactors[i].size(); ++j) {
      cout << std::setw(10) << social.contactfactors[i][j] << " ";
    }
    cout << "\n";
  }

  cout << "\n";

  // Print touchfactors with row and column labels
  cout << "touchfactors (" << social.touch_rows.size() << "x" << social.age_columns.size() << "):\n";
  cout << "           ";
  for (const auto& col : social.age_columns) {
    cout << std::setw(10) << col << " ";
  }
  cout << "\n";

  for (size_t i = 0; i < social.touchfactors.size(); ++i) {
    cout << std::setw(10) << social.touch_rows[i] << " ";
    for (size_t j = 0; j < social.touchfactors[i].size(); ++j) {
      cout << std::setw(10) << social.touchfactors[i][j] << " ";
    }
    cout << "\n";
  }
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
struct model_params {
  GeoData geodata;
  json variantdata;
  socialparams socialdata;  // Changed from json to socialparams
  json vaccinesdata;
  json vaxsched;  // this will need to be loades separately--not part of building model
};

// free function for factory pattern
model_params load_model_params(string geo_path, string variants_path,
                               string social_path, string vaxsched_path)
{
  // Use aggregate initialization to construct model_params with all members at once
  // This avoids assignment to socialdata (which has const members)
  // note the curly braces: this is initialization, NOT a call to the default constructor
  return model_params{
    .geodata = load_geodata_csv(geo_path),
    .variantdata = load_json_params(variants_path),
    .socialdata = load_social_params(social_path),
    .vaccinesdata = load_json_params(vaccines_path),
    .vaxsched = load_json_params(vaxsched_path)
  };
}
