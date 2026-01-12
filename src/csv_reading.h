#ifndef CSV_READING_H
#define CSV_READING_H

#include <string>

// Forward declarations or struct definitions
struct GeoData {
    // your struct members
};

// Function declarations
int test_csv_read(const std::string& fname);
void print_summary(const GeoData& data);
void load_csv(const std::string& filename, GeoData& data);

#endif