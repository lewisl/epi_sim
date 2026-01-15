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
const fs::path variants_fname = "variants.json";
const fs::path geodata_fname = "geo2data.csv";

// Full paths
const string variants_path = (project_dir / param_dir / variants_fname).string();
const string geodata_path = (project_dir / param_dir / geodata_fname).string();


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
    std::cout << "Loaded " << data.num_rows << " rows with " 
              << data.column_names.size() << " columns:\n\n";
    
    for (const auto& col_name : data.column_names) {
        std::cout << "  " << std::setw(12) << std::left << col_name 
                  << " (" << data.get_type(col_name) << ")\n";
    }
    
    std::cout << "\nFirst 3 rows:\n";
    for (size_t i = 0; i < std::min(size_t(3), data.num_rows); ++i) {
        std::cout << "\nRow " << i << ":\n";
        std::cout << "  fips: " << data.fips[i] << "\n";
        std::cout << "  county: " << data.county[i] << "\n";
        std::cout << "  city: " << data.city[i] << "\n";
        std::cout << "  state: " << data.state[i] << "\n";
        std::cout << "  sizecat: " << data.sizecat[i] << "\n";
        std::cout << "  pop: " << data.pop[i] << "\n";
        std::cout << "  density: " << data.density[i] << "\n";
        std::cout << "  anchor: " << data.anchor[i] << "\n";
    }
}

//
// variant data supplies infectset, progressionset, trvec, variantlist
//

int load_variant_json(string fpath) {

  try {
    std::ifstream fcontent(fpath);
    json data = json::parse(fcontent);

    cout << "\n================\nexamining json data\n";
    // cout << "================ the input json text =============\n";
    // cout << json::parse(fcontent) << "\n";
    cout << "============== length of the parsed json: " << data.size() << "\n";    
    cout << data.dump(2) << "\n";
    cout << "=== end of the json data ===\n";

    // let's see what or how we can select parts
    for (auto element : data.items()) {  // items() required for iteration of key, value
      cout << "==== " << element.key() << " ====\n";
      cout << data[element.key()]["spread"]["sendrisk"] << "\n";
      cout << "===================================\n";
    }
  }
  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
      return 1;
  }
  return 0;
}


//
// vaccine data
//



//
// vaccination schedules
//


//
// social spreading data
//


// container for all of the parameters required for a model
struct model_params {
  GeoData geodata;
  json variantdata;

  // big ass constructor
model_params(string param_dir, string geo_fname,
            string variants_fname)
{



  
  
}
};