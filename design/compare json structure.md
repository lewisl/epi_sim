param         json structure             loader

OK:
config:       outer {}, use keys         access by each individual key   
socialparams: outer {}, use keys         access keys with predefined strings
soc_dist:     outer [], "sd_cases": []   iterate over items in sd_cases arr
                                            using for range; push_back into array
seed:         outer {}, "seed_cases": [] iterate over items in seed_cases arr
                                            using for range; push_back into array


REVIEW:
variants:     outer {}, use keys         iterates over toplevel
rings:        outer {}, named array      check array, iterate array idxs
vaccines:     outer {},                  iterate over keys using .items(),
                                            use keys as names
vaxsched:     outer {}, each sched       iterate like map key, value