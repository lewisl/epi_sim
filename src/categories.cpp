#include "categories.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

// create Category instances for each kind of category
const CATEGORY STATUS {.num = {1, 2, 3, 4},
                        .name = {"unexposed", "infectious", "recovered", "dead"}};

const CATEGORY CONDITION {.num = {1, 2, 3, 4, 5},
                            .name = {"uninfected", "nil", "mild", "sick", "severe"}};

const CATEGORY AGEGRP {.num = {1, 2, 3, 4, 5},
                        .name = {"age0_19", "age20_39", "age40_59", "age60_79", "age80_up"}};

const CATEGORY VACCINE {.num = {1, 2, 3, 4},
                          .name = {"none", "Pfizer", "Moderna", "J & J"}};

// category constants defined using functor
// status
const uint8_t UNEXPOSED = STATUS("unexposed"); 
const uint8_t INFECTIOUS = STATUS("infectious");
const uint8_t RECOVERED = STATUS("recovered");
const uint8_t DEAD = STATUS("dead");

// condition
const uint8_t UNINFECTED = CONDITION("uninfected");
const uint8_t NIL = CONDITION("nil");
const uint8_t MILD = CONDITION("mild");
const uint8_t SICK = CONDITION("sick");
const uint8_t SEVERE = CONDITION("severe");
