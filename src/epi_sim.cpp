#include "epi_sim.h"
#include "categories.h"
#include "csv_reading.h"
#include <iostream>
using namespace std;

std::string fname = "/Users/lewislevin/code/Covid Modeling/Covid-ILM/sample_parameters/geo2data.csv";

int main() {
  // run_category_tests();
  int res = test_csv_read(fname);
  cout << "result of reading csv " << res << endl;
  return 0;
}