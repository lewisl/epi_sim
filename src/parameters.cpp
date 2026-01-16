#include <csv2/reader.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <filesystem>
// #include <yaml-cpp/node/parse.h>
// #include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using std::cout;
using std::string;
using std::vector;
namespace fs = std::filesystem;

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
// geodata
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
    vector<int> density;
    vector<string> anchor;      // Date stored as string (could parse to date type)
    vector<string> indoor_st;   // Date stored as string
    vector<string> indoor_end;  // Date stored as string
    
    // Helper to get type name for a column
    std::string get_type(const std::string& col_name) const {
        if (col_name == "fips" || col_name == "sizecat" || 
            col_name == "pop" || col_name == "density") {
            return "int";
        }
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
    data.density.push_back(std::stoi(values[6]));
    data.anchor.push_back(values[7]);
    data.indoor_st.push_back(values[8]);
    data.indoor_end.push_back(values[9]);
      
    data.num_rows++;
  }
  return data;
}

// as we develop more parameter classes, we'll have a print method in each 
void print_geodata(const GeoData& data) {
    cout << "Loaded " << data.num_rows << " rows with " 
              << data.column_names.size() << " columns:\n\n";

    // header
        cout << std::left;
        cout << std::setw(4) << "" << std::setw(8) << "fips";
        cout << std::setw(18) << "county";
        cout << std::setw(18) << "city";
        cout << std::setw(8) << "state";
        cout << std::setw(10) << "sizecat";
        cout << std::setw(10) << "pop";
        cout << std::setw(10) << "density";
        cout << std::setw(12) << "anchor";
        cout << "\n";
    for (size_t i = 0; i < data.num_rows; ++i) {
        cout << std::setw(4) << (std::to_string(i) + ":");
        cout << std::setw(8) << data.fips[i];
        cout << std::setw(18) << data.county[i];
        cout << std::setw(18) << data.city[i];
        cout << std::setw(8) << data.state[i];
        cout << std::setw(10) << data.sizecat[i];
        cout << std::setw(10) << data.pop[i];
        cout << std::setw(10) << data.density[i];
        cout << std::setw(12) << data.anchor[i];
        cout << "\n";
    }
}

//
// variant data supplies infectset, progressionset, trvec, variantlist
//

json load_variant_json(string fpath) {

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
    cout << "\n\n================\nexamining json data"<< ", count of top-level nodes: " << v.size() << "\n";    


    // let's see what or how we can select parts
    for (auto element : v.items()) {  // items() required for iteration of key, value
      cout << "==== " << element.key() << " ====\n";
      cout << v[element.key()]["spread"]["sendrisk"] << "\n";
      cout << "===================================\n";
    }
}

//
// vaccine data
//

json load_vaccines_json(string fpath) {

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


json load_vax_sched_json(string fpath) {
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
// social spreading data
//

json load_social_json(string fpath) {

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


void print_social_data(json v) {

    string age_choice = "age0_19";  // just for testing
    cout << "\n\n================\nexamining social data"<< ", count of top-level nodes: " << v.size() << "\n";    

    // let's see what or how we can select parts
    for (auto element :
         v.items()) { // items() required for iteration of key, value
      if (element.key() == "contactfactors" || element.key() == "touchfactors") {
        cout << "==== " << element.key() << " " << age_choice << " ====\n";
        cout << v[element.key()][age_choice] << "\n";
        cout << "===================================\n";
      }
    }
}

//
// container for all of the parameters required for a model
//
struct model_params {
  GeoData geodata;
  json variantdata;
  json socialdata;
  json vaccinesdata;
  json vaxsched;  // this will need to be loades separately--not part of building model
};

// free function for factory pattern
model_params load_model_params(string geo_path, string variants_path,
                               string social_path, string vaxsched_path)
{
  model_params params;
  params.geodata = load_geodata_csv(geo_path);
  params.variantdata = load_variant_json(variants_path);
  params.socialdata = load_social_json(social_path);
  params.vaccinesdata = load_vaccines_json(vaccines_path);
  params.vaxsched = load_vax_sched_json(vaxsched_path);
  return params;
}
