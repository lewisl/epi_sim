#include <cstdint>
#include <string>
#include <stdexcept>
#include <iostream>

using std::string;

#include "epi_sim.h"
#include "categories.h"

void run_category_tests() {
    std::cout << "=== Testing CATEGORY methods ===\n\n";

    // Test that named constants are correctly defined using functor
    std::cout << "--- Testing Named Constants (defined via functor) ---\n";
    std::cout << "UNEXPOSED = " << static_cast<int>(UNEXPOSED) << " (expected 1)\n";
    std::cout << "INFECTIOUS = " << static_cast<int>(INFECTIOUS) << " (expected 2)\n";
    std::cout << "RECOVERED = " << static_cast<int>(RECOVERED) << " (expected 3)\n";
    std::cout << "DEAD = " << static_cast<int>(DEAD) << " (expected 4)\n";
    std::cout << "UNINFECTED = " << static_cast<int>(UNINFECTED) << " (expected 1)\n";
    std::cout << "NIL = " << static_cast<int>(NIL) << " (expected 2)\n";
    std::cout << "MILD = " << static_cast<int>(MILD) << " (expected 3)\n";
    std::cout << "SICK = " << static_cast<int>(SICK) << " (expected 4)\n";
    std::cout << "SEVERE = " << static_cast<int>(SEVERE) << " (expected 5)\n";
    std::cout << "\n";

    // Test round-trip: STATUS
    std::cout << "--- Round-trip Tests with STATUS ---\n";
    std::cout << "STATUS.num values: {1, 2, 3, 4}\n";
    std::cout << "STATUS.name strings: {\"unexposed\", \"infectious\", \"recovered\", \"dead\"}\n\n";

    for (size_t i = 0; i < STATUS.num.size(); ++i) {
        uint8_t n_val = STATUS.num[i];
        std::cout << "STATUS.num[" << i << "] = " << static_cast<int>(n_val) << "\n";
        std::string name = STATUS.get_name(n_val);
        std::cout << "  STATUS.get_name(" << static_cast<int>(n_val) << ") = \"" << name << "\"\n";
        uint8_t idx_back = STATUS(name);
        std::cout << "  STATUS(\"" << name << "\") = " << static_cast<int>(idx_back);
        if (idx_back == n_val) {
            std::cout << " ✓ MATCH\n";
        } else {
            std::cout << " ✗ MISMATCH (expected " << static_cast<int>(n_val) << ")\n";
        }
        std::cout << "\n";
    }

    // Test round-trip: CONDITION
    std::cout << "--- Round-trip Tests with CONDITION ---\n";
    std::cout << "CONDITION.num values: {1, 2, 3, 4, 5}\n";
    std::cout << "CONDITION.name strings: {\"uninfected\", \"nil\", \"mild\", \"sick\", \"severe\"}\n\n";

    for (size_t i = 0; i < CONDITION.num.size(); ++i) {
        uint8_t n_val = CONDITION.num[i];
        std::cout << "CONDITION.num[" << i << "] = " << static_cast<int>(n_val) << "\n";
        std::string name = CONDITION.get_name(n_val);
        std::cout << "  CONDITION.get_name(" << static_cast<int>(n_val) << ") = \"" << name << "\"\n";
        uint8_t idx_back = CONDITION(name);
        std::cout << "  CONDITION(\"" << name << "\") = " << static_cast<int>(idx_back);
        if (idx_back == n_val) {
            std::cout << " ✓ MATCH\n";
        } else {
            std::cout << " ✗ MISMATCH (expected " << static_cast<int>(n_val) << ")\n";
        }
        std::cout << "\n";
    }

    // Test round-trip: AGEGRP
    std::cout << "--- Round-trip Tests with AGEGRP ---\n";
    std::cout << "AGEGRP.num values: {1, 2, 3, 4, 5}\n";
    std::cout << "AGEGRP.name strings: {\"age0_19\", \"age20_39\", \"age40_59\", \"age60_79\", \"age80_up\"}\n\n";

    for (size_t i = 0; i < AGEGRP.num.size(); ++i) {
        uint8_t n_val = AGEGRP.num[i];
        std::cout << "AGEGRP.num[" << i << "] = " << static_cast<int>(n_val) << "\n";
        std::string name = AGEGRP.get_name(n_val);
        std::cout << "  AGEGRP.get_name(" << static_cast<int>(n_val) << ") = \"" << name << "\"\n";
        uint8_t idx_back = AGEGRP(name);
        std::cout << "  AGEGRP(\"" << name << "\") = " << static_cast<int>(idx_back);
        if (idx_back == n_val) {
            std::cout << " ✓ MATCH\n";
        } else {
            std::cout << " ✗ MISMATCH (expected " << static_cast<int>(n_val) << ")\n";
        }
        std::cout << "\n";
    }

    // Test error handling
    std::cout << "--- Error Handling Tests ---\n";

    try {
        std::cout << "Attempting STATUS.get_name(10) (out of bounds)...\n";
        STATUS.get_name(10);
        std::cout << "ERROR: Should have thrown exception!\n";
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception: " << e.what() << "\n";
    }

    try {
        std::cout << "Attempting STATUS(\"invalid\") (not found)...\n";
        STATUS("invalid");
        std::cout << "ERROR: Should have thrown exception!\n";
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected exception: " << e.what() << "\n";
    }

    std::cout << "\n=== All tests completed ===\n";
}

int main() {
  run_category_tests();
  return 0;
}