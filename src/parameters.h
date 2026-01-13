#ifndef CSV_READING_H
#define CSV_READING_H

#include <string>
#include <vector>

using std::vector;
using std::string;

// Forward declarations or struct definitions
struct GeoData {
    // Column metadata
    vector<string> column_names = {
        "fips", "county", "city", "state", "sizecat", 
        "pop", "density", "anchor", "indoor_st", "indoor_end"
    };
    
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
    
    size_t num_rows = 0;
    
    // Helper to get type name for a column
    std::string get_type(const std::string& col_name) const {
        if (col_name == "fips" || col_name == "sizecat" || 
            col_name == "pop" || col_name == "density") {
            return "int";
        }
        return "string";
    }
};

// Function declarations
int test_csv_read(const std::string& fname);
void print_summary(const GeoData& data);
void load_csv(const std::string& filename, GeoData& data);

#endif