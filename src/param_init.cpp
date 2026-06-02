#include "helpers.h"
#include <cctype>
#include <cstdlib>
#include <vector>
#include <string>
#include "cases.h"
#include "epi_sim.h"
#include "setup.h"
#include "sim.h"
#include <absl/strings/str_split.h>
#include "parameters.h"
#include <fmt/args.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

namespace fs = std::filesystem;

namespace {

std::string config_json = R"TAG({
    "days": 180,                    // number of days the simulation runs
    "locale": 38015,                // cusip location for the population and other characteristics
    "calendar_start": "2020-01-01", // day 1 of the simulation is set to this actual calendar date
    "dovax": false,                 // adminster vaccines: true or false
    "debug": false,                 // print verbose debug diagnostics such as overflow warnings
    "geodata": "geo2data.csv",
    "variants": "variants.json",
    "social": "socialparams.json",
    "vaccines": "vaccines.json",
    "vax_sched_dir": "vaccine_100k"
}
)TAG";

std::string socialparams_json = R"TAG({
  "gammashape": 1.0,
  "indoor_uplift": 1.1,
  "contactfactors": {
      "age0_19": {
          "nil": 1.045,
          "mild": 1.045,
          "sick": 0.665,
          "severe": 0.475
      },
      "age20_39": {
          "nil": 1.995,
          "mild": 1.9,
          "sick": 0.95,
          "severe": 0.57
      },
      "age40_59": {
          "nil": 1.995,
          "mild": 1.9,
          "sick": 0.95,
          "severe": 0.57
      },
      "age60_79": {
          "nil": 1.615,
          "mild": 1.52,
          "sick": 0.665,
          "severe": 0.475
      },
      "age80_up": {
          "nil": 0.95,
          "mild": 0.855,
          "sick": 0.57,
          "severe": 0.475
      }
  },
  "touchfactors": {
      "age0_19": {
          "unexposed": 0.55,
          "recovered": 0.55,
          "nil": 0.55,
          "mild": 0.55,
          "sick": 0.28,
          "severe": 0.18
      },
      "age20_39": {
          "unexposed": 0.63,
          "recovered": 0.63,
          "nil": 0.63,
          "mild": 0.62,
          "sick": 0.35,
          "severe": 0.18
      },
      "age40_59": {
          "unexposed": 0.61,
          "recovered": 0.61,
          "nil": 0.61,
          "mild": 0.58,
          "sick": 0.3,
          "severe": 0.18
      },
      "age60_79": {
          "unexposed": 0.41,
          "recovered": 0.41,
          "nil": 0.41,
          "mild": 0.41,
          "sick": 0.18,
          "severe": 0.18
      },
      "age80_up": {
          "unexposed": 0.35,
          "recovered": 0.35,
          "nil": 0.35,
          "mild": 0.28,
          "sick": 0.18,
          "severe": 0.18
      }
  }
}
)TAG";

std::string variants_json = R"TAG({
{
    "base"         : {
        "spread"             : {
            "sendrisk"      : [
                0   , 0.3 , 0.65, 0.75, 0.85, 0.85, 0.75, 0.7 , 0.65, 0.6 , 0.5 , 0.2 , 0.1 , 0.1 , 0.1 , 0.05, 0.05,
                0   , 0   , 0   , 0   , 0   , 0   , 0   , 0
            ],
            "recvrisk"      : [0.1, 0.39, 0.44, 0.54, 0.56],
            "basemultiplier": 1
        },
        "immunity"           : {
            "recovery_immunity": {
                "base"         : 0.8,
                "alpha"        : 0.8,
                "delta"        : 0.6,
                "omicron_ba1"  : 0.6,
                "omicron_ba2"  : 0.6,
                "omicron_ba4_5": 0.4
            },
            "immunehalflife"   : 360
        },
        "progression_tree"   : {
            "age0_19" : {
                "5" : {
                    "nil"   : [0, 0.4, 0.5, 0.1, 0, 0],
                    "mild"  : [0, 0.3, 0.6, 0.1, 0, 0],
                    "sick"  : [0, 0  , 0  , 1  , 0, 0],
                    "severe": [0, 0  , 0  , 0  , 1, 0]
                },
                "9" : {
                    "nil"   : [0.9, 0, 0  , 0.1 , 0   , 0],
                    "mild"  : [0.4, 0, 0.6, 0   , 0   , 0],
                    "sick"  : [0  , 0, 0  , 0.95, 0.05, 0],
                    "severe": [0  , 0, 0  , 0   , 1   , 0]
                },
                "14": {
                    "nil"   : [1    , 0, 0, 0   , 0    , 0    ],
                    "mild"  : [1    , 0, 0, 0   , 0    , 0    ],
                    "sick"  : [0.85 , 0, 0, 0.12, 0.03 , 0    ],
                    "severe": [0.692, 0, 0, 0   , 0.302, 0.006]
                },
                "19": {
                    "nil"   : [1    , 0, 0, 0, 0    , 0    ],
                    "mild"  : [1    , 0, 0, 0, 0    , 0    ],
                    "sick"  : [0    , 0, 0, 1, 0    , 0    ],
                    "severe": [0.891, 0, 0, 0, 0.106, 0.003]
                },
                "25": {
                    "nil"   : [1    , 0, 0, 0, 0, 0    ],
                    "mild"  : [1    , 0, 0, 0, 0, 0    ],
                    "sick"  : [0.976, 0, 0, 0, 0, 0.024],
                    "severe": [0.91 , 0, 0, 0, 0, 0.09 ]
                }
            },
            "age20_39": {
                "5" : {
                    "nil"   : [0, 0.2 , 0.7 , 0.1, 0, 0],
                    "mild"  : [0, 0.15, 0.85, 0  , 0, 0],
                    "sick"  : [0, 0   , 0   , 1  , 0, 0],
                    "severe": [0, 0   , 0   , 1  , 0, 0]
                },
                "9" : {
                    "nil"   : [0.9 , 0, 0, 0.1 , 0  , 0],
                    "mild"  : [0.85, 0, 0, 0.15, 0  , 0],
                    "sick"  : [0   , 0, 0, 0.9 , 0.1, 0],
                    "severe": [0   , 0, 0, 0   , 1  , 0]
                },
                "14": {
                    "nil"   : [1    , 0, 0, 0  , 0    , 0    ],
                    "mild"  : [1    , 0, 0, 0  , 0    , 0    ],
                    "sick"  : [0.83 , 0, 0, 0.1, 0.07 , 0    ],
                    "severe": [0.474, 0, 0, 0  , 0.514, 0.012]
                },
                "19": {
                    "nil"   : [1    , 0, 0, 0, 0    , 0    ],
                    "mild"  : [1    , 0, 0, 0, 0    , 0    ],
                    "sick"  : [0.922, 0, 0, 0, 0.072, 0.006],
                    "severe": [0.922, 0, 0, 0, 0.072, 0.006]
                },
                "25": {
                    "nil"   : [1    , 0, 0, 0, 0, 0    ],
                    "mild"  : [1    , 0, 0, 0, 0, 0    ],
                    "sick"  : [0.964, 0, 0, 0, 0, 0.036],
                    "severe": [0.964, 0, 0, 0, 0, 0.036]
                }
            },
            "age40_59": {
                "5" : {
                    "nil"   : [0, 0.2 , 0.7 , 0.1, 0, 0],
                    "mild"  : [0, 0.15, 0.85, 0  , 0, 0],
                    "sick"  : [0, 0   , 0   , 1  , 0, 0],
                    "severe": [0, 0   , 0   , 0  , 1, 0]
                },
                "9" : {
                    "nil"   : [0.9 , 0, 0   , 0.1, 0  , 0],
                    "mild"  : [0.85, 0, 0.05, 0.1, 0  , 0],
                    "sick"  : [0   , 0, 0   , 0.9, 0.1, 0],
                    "severe": [0   , 0, 0   , 0  , 1  , 0]
                },
                "14": {
                    "nil"   : [1    , 0, 0, 0   , 0    , 0    ],
                    "mild"  : [0.9  , 0, 0, 0.1 , 0    , 0    ],
                    "sick"  : [0.85 , 0, 0, 0.14, 0.01 , 0    ],
                    "severe": [0.776, 0, 0, 0   , 0.206, 0.018]
                },
                "19": {
                    "nil"   : [1    , 0, 0, 0, 0    , 0    ],
                    "mild"  : [1    , 0, 0, 0, 0    , 0    ],
                    "sick"  : [0.856, 0, 0, 0, 0.126, 0.018],
                    "severe": [0.856, 0, 0, 0, 0.126, 0.018]
                },
                "25": {
                    "nil"   : [1    , 0, 0, 0, 0, 0    ],
                    "mild"  : [1    , 0, 0, 0, 0, 0    ],
                    "sick"  : [0.958, 0, 0, 0, 0, 0.042],
                    "severe": [0.958, 0, 0, 0, 0, 0.042]
                }
            },
            "age60_79": {
                "5" : {
                    "nil"   : [0, 0.15, 0.6, 0.25, 0, 0],
                    "mild"  : [0, 0   , 0.7, 0.3 , 0, 0],
                    "sick"  : [0, 0   , 0  , 1   , 0, 0],
                    "severe": [0, 0   , 0  , 0   , 1, 0]
                },
                "9" : {
                    "nil"   : [0.62, 0, 0   , 0.38, 0   , 0],
                    "mild"  : [0.5 , 0, 0.25, 0.25, 0   , 0],
                    "sick"  : [0   , 0, 0   , 0.78, 0.22, 0],
                    "severe": [0   , 0, 0   , 0   , 1   , 0]
                },
                "14": {
                    "nil"   : [0.8  , 0.1, 0.1 , 0   , 0    , 0   ],
                    "mild"  : [0.8  , 0  , 0.15, 0.05, 0    , 0   ],
                    "sick"  : [0.8  , 0  , 0   , 0.1 , 0.1  , 0   ],
                    "severe": [0.165, 0  , 0   , 0   , 0.715, 0.12]
                },
                "19": {
                    "nil"   : [1   , 0, 0, 0, 0   , 0   ],
                    "mild"  : [1   , 0, 0, 0, 0   , 0   ],
                    "sick"  : [0.81, 0, 0, 0, 0.13, 0.06],
                    "severe": [0.81, 0, 0, 0, 0.13, 0.06]
                },
                "25": {
                    "nil"   : [1    , 0, 0, 0, 0, 0    ],
                    "mild"  : [1    , 0, 0, 0, 0, 0    ],
                    "sick"  : [0.76 , 0, 0, 0, 0, 0.24 ],
                    "severe": [0.688, 0, 0, 0, 0, 0.312]
                }
            },
            "age80_up": {
                "5" : {
                    "nil"   : [0, 0.1, 0.5, 0.4, 0  , 0],
                    "mild"  : [0, 0.1, 0.5, 0.4, 0  , 0],
                    "sick"  : [0, 0.1, 0.5, 0.4, 0  , 0],
                    "severe": [0, 0  , 0  , 0.4, 0.6, 0]
                },
                "9" : {
                    "nil"   : [0.5, 0, 0  , 0.5, 0  , 0],
                    "mild"  : [0  , 0, 0.4, 0.6, 0  , 0],
                    "sick"  : [0  , 0, 0  , 0.6, 0.4, 0],
                    "severe": [0  , 0, 0  , 0  , 1  , 0]
                },
                "14": {
                    "nil"   : [0.7 , 0, 0.3, 0  , 0   , 0   ],
                    "mild"  : [0.7 , 0, 0  , 0.3, 0   , 0   ],
                    "sick"  : [0.7 , 0, 0  , 0.1, 0.2 , 0   ],
                    "severe": [0.12, 0, 0  , 0  , 0.67, 0.21]
                },
                "19": {
                    "nil"   : [1   , 0, 0, 0, 0   , 0   ],
                    "mild"  : [1   , 0, 0, 0, 0   , 0   ],
                    "sick"  : [0   , 0, 0, 0, 1   , 0   ],
                    "severe": [0.49, 0, 0, 0, 0.24, 0.27]
                },
                "25": {
                    "nil"   : [1    , 0, 0, 0, 0, 0    ],
                    "mild"  : [1    , 0, 0, 0, 0, 0    ],
                    "sick"  : [0.682, 0, 0, 0, 0, 0.318],
                    "severe": [0.676, 0, 0, 0, 0, 0.324]
                }
            }
        },
        "progression_factors": { 
            "riskadjust": [],
            "vaxhalflifeadjust": {"JnJ": 0.5, "Pfizer": 0.8, "Moderna": 0.9} }
    },
    "alpha"        : {
        "spread"             : {"sendrisk": [], "recvrisk": [], "basemultiplier": 1.1},
        "immunity"           : {
            "recovery_immunity": {
                "base"         : 0.8,
                "alpha"        : 0.8,
                "delta"        : 0.7,
                "omicron_ba1"  : 0.6,
                "omicron_ba2"  : 0.6,
                "omicron_ba4_5": 0.4
            },
            "immunehalflife"   : 360
        },
        "progression_tree"   : null,
        "progression_factors": {
            "riskadjust"       : [1, 1, 1, 1.1, 1.1, 1.1],
            "vaxhalflifeadjust": {"JnJ": 0.5, "Pfizer": 0.8, "Moderna": 0.9}
        }
    },
    "delta"        : {
        "spread"             : {"sendrisk": [], "recvrisk": [], "basemultiplier": 1.2},
        "immunity"           : {
            "recovery_immunity": {
                "base"         : 0.8,
                "alpha"        : 0.8,
                "delta"        : 0.9,
                "omicron_ba1"  : 0.6,
                "omicron_ba2"  : 0.6,
                "omicron_ba4_5": 0.4
            },
            "immunehalflife"   : 360
        },
        "progression_tree"   : null,
        "progression_factors": {
            "riskadjust"       : [1, 1, 1, 1.1, 1.1, 1.1],
            "vaxhalflifeadjust": {"JnJ": 0.9, "Pfizer": 0.9, "Moderna": 0.9}
        }
    },
    "omicron_ba1"  : {
        "spread"             : {
            "sendrisk"      : [
                0    , 0.445, 0.975, 1.125, 1.275, 1.275, 1.125, 1.05 , 0.975, 0.9  , 0.75 , 0.5  , 0.1  , 0.05 ,
                0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0
            ],
            "recvrisk"      : [0.1, 0.39, 0.44, 0.54, 0.56],
            "basemultiplier": 1.2
        },
        "immunity"           : {
            "recovery_immunity": {
                "base"         : 0.95,
                "alpha"        : 0.95,
                "delta"        : 0.8 ,
                "omicron_ba1"  : 0.8 ,
                "omicron_ba2"  : 0.8 ,
                "omicron_ba4_5": 0.6
            },
            "immunehalflife"   : 360
        },
        "progression_tree"   : null,
        "progression_factors": {
            "riskadjust"       : [1.1, 1.05, 1.05, 0.9, 0.7, 0.6],
            "vaxhalflifeadjust": {"JnJ": 0.5, "Pfizer": 0.8, "Moderna": 0.9}
        }
    },
    "omicron_ba2"  : {
        "spread"             : {
            "sendrisk"      : [
                0    , 0.545, 1.175, 1.325, 1.475, 1.475, 1.325, 1.25 , 1.075, 0.9  , 0.75 , 0.5  , 0.1  , 0.05 ,
                0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0
            ],
            "recvrisk"      : [0.1, 0.39, 0.44, 0.54, 0.56],
            "basemultiplier": 1.3
        },
        "immunity"           : {
            "recovery_immunity": {
                "base"         : 0.95,
                "alpha"        : 0.95,
                "delta"        : 0.8 ,
                "omicron_ba1"  : 0.8 ,
                "omicron_ba2"  : 0.8 ,
                "omicron_ba4_5": 0.6
            },
            "immunehalflife"   : 360
        },
        "progression_tree"   : null,
        "progression_factors": {
            "riskadjust"       : [1.1, 1.05, 1.05, 0.9, 0.5, 0.6],
            "vaxhalflifeadjust": {"JnJ": 0.5, "Pfizer": 0.8, "Moderna": 0.9}
        }
    },
    "omicron_ba4_5": {
        "spread"             : {
            "sendrisk"      : [
                0    , 0.545, 1.175, 1.325, 1.475, 1.475, 1.325, 1.25 , 1.075, 0.9  , 0.75 , 0.5  , 0.1  , 0.05 ,
                0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0    , 0
            ],
            "recvrisk"      : [0.1, 0.39, 0.44, 0.54, 0.56],
            "basemultiplier": 1.4
        },
        "immunity"           : {
            "recovery_immunity": {
                "base"         : 0.95,
                "alpha"        : 0.95,
                "delta"        : 0.8 ,
                "omicron_ba1"  : 0.8 ,
                "omicron_ba2"  : 0.8 ,
                "omicron_ba4_5": 0.75
            },
            "immunehalflife"   : 360
        },
        "progression_tree"   : null,
        "progression_factors": {
            "riskadjust"       : [1.1, 1.05, 1.05, 0.9, 0.5, 0.6],
            "vaxhalflifeadjust": {"JnJ": 0.5, "Pfizer": 0.8, "Moderna": 0.9}
        }
    }
}

}
)TAG";

std::string vaccines_json = R"TAG(
{
    "Pfizer": {
        "halflife": 360,
        "reqdshots": 2,
        "delay2ndshot": 21,
        "delaybooster": 160,
        "full_effect_days": 14,
        "day1_effect": 0.65,
        "infectfactor": {
            "base": 0.9,
            "alpha": 0.9,
            "delta": 0.85,
            "omicron_ba1": 0.75,
            "omicron_ba2": 0.75,
            "omicron_ba4_5": 0.75
        },
        "effectiveness": {
            "first": {
                "base": 0.9,
                "alpha": 0.9,
                "delta": 0.7,
                "omicron_ba1": 0.65,
                "omicron_ba2": 0.65,
                "omicron_ba4_5": 0.65
            },
            "full": {
                "base": 0.94,
                "alpha": 0.94,
                "delta": 0.8,
                "omicron_ba1": 0.85,
                "omicron_ba2": 0.85,
                "omicron_ba4_5": 0.85
            },
            "booster": {
                "base": 0.94,
                "alpha": 0.94,
                "delta": 0.8,
                "omicron_ba1": 0.85,
                "omicron_ba2": 0.85,
                "omicron_ba4_5": 0.85
            }
        }
    },
    "Moderna": {
        "halflife": 360,
        "reqdshots": 2,
        "delay2ndshot": 21,
        "delaybooster": 160,
        "full_effect_days": 14,
        "day1_effect": 0.65,
        "infectfactor": {
            "base": 0.9,
            "alpha": 0.9,
            "delta": 0.85,
            "omicron_ba1": 0.75,
            "omicron_ba2": 0.75,
            "omicron_ba4_5": 0.75
        },
        "effectiveness": {
            "first": {
                "base": 0.92,
                "alpha": 0.92,
                "delta": 0.85,
                "omicron_ba1": 0.6,
                "omicron_ba2": 0.6,
                "omicron_ba4_5": 0.6
            },
            "full": {
                "base": 0.95,
                "alpha": 0.95,
                "delta": 0.88,
                "omicron_ba1": 0.75,
                "omicron_ba2": 0.75,
                "omicron_ba4_5": 0.75
            },
            "booster": {
                "base": 0.95,
                "alpha": 0.95,
                "delta": 0.88,
                "omicron_ba1": 0.8,
                "omicron_ba2": 0.8,
                "omicron_ba4_5": 0.8
            }
        }
    },
    "JnJ": {
        "halflife": 360,
        "reqdshots": 1,
        "delay2ndshot": 0,
        "delaybooster": 160,
        "full_effect_days": 14,
        "day1_effect": 0.65,
        "infectfactor": {
            "base": 0.85,
            "alpha": 0.85,
            "delta": 0.75,
            "omicron_ba1": 0.65,
            "omicron_ba2": 0.65,
            "omicron_ba4_5": 0.65
        },
        "effectiveness": {
            "first": {
                "base": 0.88,
                "alpha": 0.88,
                "delta": 0.65,
                "omicron_ba1": 0.75,
                "omicron_ba2": 0.75,
                "omicron_ba4_5": 0.75
            },
            "full": {
                "base": 0.88,
                "alpha": 0.88,
                "delta": 0.65,
                "omicron_ba1": 0.75,
                "omicron_ba2": 0.75,
                "omicron_ba4_5": 0.75
            },
            "booster": {
                "base": 0.88,
                "alpha": 0.88,
                "delta": 0.65,
                "omicron_ba1": 0.75,
                "omicron_ba2": 0.75,
                "omicron_ba4_5": 0.75
            }
        }
    }
}
)TAG";

std::string geodata_csv = R"TAG(
fips,county,city,state,sizecat,pop,density,anchor,indoor_st,indoor_end
6075,San Francisco,San Francisco,CA,2,881549,17255,2020-02-01,0001-09-15,0002-05-30
53033,Seattle,Seattle,WA,2,2252782,5175,2020-02-01,0001-09-15,0002-05-30
36061,New York,New York,NY,1,8336817,40306,2020-02-01,0001-09-15,0002-05-30
39035,Cuyahoga,Cleveland,OH,2,1235072,3063,2020-02-01,0001-09-15,0002-05-30
48113,Dallas,Dallas,TX,2,2635516,4000,2020-02-01,0001-09-15,0002-05-30
39151,Stark,Canton,OH,3,370606,1688,2020-02-01,0001-09-15,0002-05-30
34013,Essex,Newark,NJ,3,798975,6396,2020-02-01,0001-09-15,0002-05-30
13089,DeKalb,Atlanta,GA,2,1063937,2708,2020-02-01,0001-09-15,0002-05-30
17167,Sangamon,Springfield,IL,3,194672,1747,2020-02-01,0001-09-15,0002-05-30
38015,Burleigh,Bismarck,ND,3,95626,2157,2020-02-01,0001-09-15,0002-05-30
4013,Maricopa,Phoenix,AZ,1,4485414,2798,2020-03-01,0001-09-15,0002-05-30
42003,Allegheny,Pittsburgh,PA,2,1216045,5461,2020-02-01,0001-09-15,0002-05-30
27053,Hennepin,Minneapolis,MN,2,1265843,7821,2020-02-01,0001-09-15,0002-05-30
31055,Douglas,Omaha,NE,2,571327,3378,2020-02-01,0001-09-15,0002-05-30
8031,Denver,Denver,CO,2,727211,4520,2020-02-01,0001-09-15,0002-05-30
)TAG";

std::string rings_json = R"TAG(
{
    "rings": [
        {
            "name"                   : "ring_1",
            "pct_of_population"      : 0.4,
            "out_ring_prob_by_agegrp": {
                "age0_19" : 0.05,
                "age20_39": 0.10,
                "age40_59": 0.15,
                "age60_79": 0.10,
                "age80_up": 0.05
            }
        },
        {
            "name"                   : "ring_2",
            "pct_of_population"      : 0.6,
            "out_ring_prob_by_agegrp": {
                "age0_19" : 0.20,
                "age20_39": 0.25,
                "age40_59": 0.30,
                "age60_79": 0.20,
                "age80_up": 0.15
            }
        }
    ]
}
)TAG";

} // namespace

void write_file(const std::string& content, std::string filename, std::string extension,
  fs::path path_name) {

    // fs::path output_path = std::getenv("HOME");
    // for (auto step : path_steps) {
    //   output_path /= step;
    // }

    filename.append(extension);
    std::ofstream out(path_name / filename);
    if (!out) {
        throw std::runtime_error(
            fmt::format("Could not file to '{}'", path_name.string()));
    }

    out << content;
}


void create_scaffold() {

  try {
    vector<string> case_path_steps;
    fs::path case_dir;
    json jdata;

    // read user's output dir from cases.json.  error if not found or not writable
    std::filesystem::path casedirpath = std::filesystem::current_path() / "casedir.json";
    jdata = load_json_params(casedirpath );
    case_path_steps = absl::StrSplit(string(jdata["case_directory"]), '/'); 
    const char* home = std::getenv("HOME");
    if (!home) throw std::runtime_error("HOME not set");
    std::string first_step = case_path_steps[0] == "~" ? home : case_path_steps[0];
    case_dir /= first_step;
    for (size_t step_idx = 1; step_idx < case_path_steps.size(); ++step_idx) {
      case_dir /= case_path_steps[step_idx];
    }

    std::cout << case_dir << std::endl;

    // create case dir, inputs subdir, outputs subdir
    if (std::filesystem::exists(case_dir)) {
      fmt::println("Directory for your case {} already exists.  Ending...", case_dir);
      exit(1);
    }

      fs::create_directories(case_dir);
      fs::create_directories(case_dir / "inputs");
      fs::create_directories(case_dir / "outputs");
    

    // write input files to inputs subdir
      auto in = case_dir / "inputs";
      write_file(config_json,      "config",      in, "json");
      write_file(socialparams_json,"socialparams",in, "json");
      write_file(variants_json,    "variants",    in, "json");
      write_file(vaccines_json,    "vaccines",    in, "json");
      write_file(geodata_csv,      "geodata",     in, "csv");
      write_file(rings_json,       "rings",       in, "json");
  }
    catch (const fs::filesystem_error& e) {
        fmt::println("filesystem error: {} (path: {})", e.what(), e.path1().string());
        exit(1);
    }
    catch (const std::exception& e) {
        fmt::println("error creating scaffold: {}", e.what());
        exit(1);
    }
}