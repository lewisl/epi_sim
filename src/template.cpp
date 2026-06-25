#include "template.h"

const std::string config_json = R"TAG({
    "days": 180,                      // number of days the simulation runs
    "locale": 38015,                  // cusip location for population and other characteristics, must match a row in geodata
    "calendar_start": "2020-01-01",   // day 1 of the simulation is set to this actual calendar date
    "dovax": false,                   // adminster vaccines: true or false
    "do_social_distancing": false,    // apply social distancing cases: true or false
    "do_rings": false,                // assign ring membership and use ring contact rules: true or false
    "debug": false,                   // print verbose debug diagnostics such as overflow warnings
    "age_dist": [0.251, 0.271, 0.255, 0.184, 0.039],

    "case_desc": "young: no distancing; old: rigorous; Moderna 50pct",

    "geodata":           "geodata.csv",
    "variants":          "variants.json",
    "social_params":     "socialparams.json",
    "seed":              "seed.json",

    "vaccines":          "vaccines.json",
    "vax_sched_dir":     "vaccine_100k",
    "rings":             "rings.json",
    "social_dist":       "soc_dist.json",

    "output":             "output"
}
)TAG";

const std::string socialparams_json = R"TAG({
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

const std::string variants_json = R"TAG({
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
)TAG";

const std::string seed_json = R"TAG([
    {
        "triggerday": 1,
        "startofday": true,
        "filter": [
            {"trait": "agegrp", "val": "Age20_39"}
        ],
        "change": {
            "terms": [
                {"trait": "status",   "val": "infectious"},
                {"trait": "cond",     "val": "nil"},
                {"trait": "variant",  "val": "base"},
                {"trait": "duration", "val": 1}
            ],
            "count": 3
        }
    },
    {
        "triggerday": 1,
        "startofday": true,
        "filter": [
            {"trait": "agegrp", "val": "Age40_59"}
        ],
        "change": {
            "terms": [
                {"trait": "status",   "val": "infectious"},
                {"trait": "cond",     "val": "nil"},
                {"trait": "variant",  "val": "base"},
                {"trait": "duration", "val": 1}
            ],
            "count": 3
        }
    }
]
)TAG";

const std::string social_dist_json = R"TAG([  
    {
        "name": "young",
        "startday": 50,
        "endday": 100,
        "comply": 0.9,
        "contact_delta": [0.2, 1.0],
        "touch_delta": [0.18, 0.6],
        "include_ages": []
    },
    {
        "name": "old",
        "startday": 100,
        "endday":300,
        "comply": 0.9,
        "contact_delta": [0.2, 1.0],
        "touch_delta": [0.18, 0.6],
        "include_ages": ["Age60_79", "Age80_up"]
    }
]
)TAG";


const std::string vaccines_json = R"TAG(
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

const std::string geodata_csv = R"TAG(fips,county,city,state,sizecat,pop,density,anchor,indoor_st,indoor_end
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

const std::string rings_json = R"TAG({
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

namespace vaxsched {
const std::string loc38015_old_json = R"TAG({
"vaxesincluded": {
    "Pfizer": {
        "mix": 0.5,
        "starting_doses": 40000,
        "pct2ndshot": 0.9,
        "pctboost": 0.6,
        "alternate": [
            "Moderna",
            "JnJ"
        ]
    },
    "Moderna": {
        "mix": 0.45,
        "starting_doses": 36000,
        "pct2ndshot": 0.9,
        "pctboost": 0.6,
        "alternate": [
            "Pfizer",
            "JnJ"
        ]
    },
    "JnJ": {
        "mix": 0.05,
        "starting_doses": 4000,
        "pct2ndshot": 0,
        "pctboost": 0.4,
        "alternate": [
            "Pfizer",
            "Moderna"
        ]
    }
},
"dayrange": [
    350,
    700
],
"targetpct": 0.95,
"filtervec": [
    "age80_up",
    "age60_79"
],
"shotmode": "all",
"pattern": [
    0,
    0.02,
    0.05,
    0.1,
    0.15,
    0.19,
    0.21,
    0.16,
    0.08,
    0.03,
    0.01,
    0
],
"spreadfunc": null
}
)TAG";
  
const std::string loc38015_young_json = R"TAG({
  "vaxesincluded": {
      "Pfizer": {
          "mix": 0.43,
          "starting_doses": 30000,
          "pct2ndshot": 0.8,
          "pctboost": 0.3,
          "alternate": [
              "Moderna",
              "JnJ"
          ]
      },
      "Moderna": {
          "mix": 0.34,
          "starting_doses": 24000,
          "pct2ndshot": 0.8,
          "pctboost": 0.3,
          "alternate": [
              "Pfizer",
              "JnJ"
          ]
      },
      "JnJ": {
          "mix": 0.23,
          "starting_doses": 16000,
          "pct2ndshot": 0,
          "pctboost": 0.2,
          "alternate": [
              "Pfizer",
              "Moderna"
          ]
      }
  },
  "dayrange": [
      350,
      700
  ],
  "targetpct": 0.65,
  "filtervec": [
      "age0_19",
      "age20_39",
      "age40_59"
  ],
  "shotmode": "all",
  "pattern": [
      0,
      0.02,
      0.05,
      0.1,
      0.15,
      0.19,
      0.21,
      0.16,
      0.08,
      0.03,
      0.01,
      0
  ],
  "spreadfunc": null
}
)TAG";

} // namespace vaxsched