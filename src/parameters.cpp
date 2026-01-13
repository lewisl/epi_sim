#include <csv2/reader.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <yaml-cpp/yaml.h>
#include "parameters.h"


// Struct of vectors matching your CSV structure


void load_csv(const std::string& filename, GeoData& data) {
    using namespace csv2;
    
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
        
        // Parse into typed columns (order matches CSV)
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
}

void print_summary(const GeoData& data) {
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
