#include "test_utils.h"

void test_something() {
    CHECK(1 + 1 == 2);
    CHECK(some_fn(0) == expected);
}

void test_needs_valid_ptr() try {
    auto p = make_thing();
    REQUIRE(p != nullptr);    // pointless to continue if null
    CHECK(p->value() == 42);
} catch (std::logic_error&) {}

int main() {
    test_something();
    test_needs_valid_ptr();
    test_summary();
    return test_failure_count() > 0 ? 1 : 0;
}