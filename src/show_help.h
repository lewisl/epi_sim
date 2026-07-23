#pragma once

#include <array>
#include <string_view>

void show_topics();
void show_help();
void show_help(std::string_view topic);

namespace hlptxt {

// help text to display

inline constexpr std::string_view overview_help = R"TAG(
Overview
-----------
epi_sim is an agent-based epidemic simulation model.

Agent-based means each individual in a populution is tracked during the simulation. The
simulation is controlled by sets of parameters that determine:
- the social pattern of spreading of the pathogen; 
- the transmissibility of the pathogen; 
- infected people's progression through stages of the disease;  
- the effectiveness of available vaccines;  
- schedules for administering vaccine doses; 
- the population size, density and indoor seasonality of various cities, 
- seeding the initial arrival of carriers of the pathogen; and
- social distancing patterns among people whether imposed or voluntary. 

Most of the parameters are entered in .json files and .csv for the population data. The application
will create template files with valid entries for all the parameters for a simulation case. 
Cases can be organized into a project directory or each case can be its own directory. You should edit these files 
to create the epidemic conditions you wish to simulate. 

To learn more choose the help command from the menu to exlore topics in more depth.
)TAG";

inline constexpr std::string_view project_cases_help = R"TAG(
Projects and Cases Help
------------------------
A case provides all of the input parameter files needed to run a simulation. 
The case directory contains an sub-directory `input` 
that will contain a valid template for each parameter file. 
See help /parameters for descriptions of each file. You should edit the
input parameter files to create the case you need for your simulation.
The case directory also contains an `output` sub-directory that will contain all 
the outputs from running the simulation.   

A project directory can hold many cases.   For a bit of simplicity you can also create
a single directory for one case that will contain `input` and `output`.

You can access these commands from the menu or run each command at the command line:
- Create or select a project directory: `epi_sim --set-project-dir ~/my-project`.
- Add a case to the active project: `epi_sim --init-case <case-label>`.
- List cases in the active project: `epi_sim --show-cases`.
- Run a project case: `epi_sim --run-case <case-label>`.
- Create a standalone case directory: `epi_sim --setup-dir ~/path/to/case`.
- Run a standalone case directory: `epi_sim --use-dir ~/path/to/case`.
- Use `help parameters` to learn more about the available input files.
)TAG";

inline constexpr std::string_view parameters_help = R"TAG(
Parameters Help

The following parameter files are placed in the input directory of a case directory:
- config.json      Essential parameters that drive the simulation, including:
                      - length in days;
                      - the calendar date your epidemic begins;
                      - the age distribution of your population by 20 year cohorts;
                      - a brief description
                      - the file names for the other parameter files (best to 
                        just use the provided defaults);
- geodata.csv       The population of several cities. Feel free to create one or more "fake" cities.
- variants.json     Variants of a virus pathogen and infectiousness and recovery 
                    characteristics of each variant.
- socialparams.json Behavior of population in the simulation that affects spread of the disease.
- seed.json         Number and characteristics of people who start the epidemic. You can have more than
                    one seeding event, especially to prolong the epidemic across many months.
- vaccines.json     Name and efficacy of available vaccines.
- vax_sched_dir     Explicit distribution of vaccines by time period and total available doses. The 
                    directory includes multiple schedules for different age groups and timing.
- rings.json        Contact groups for infected people spreading the disease. "Ring" simply means
                    people within a ring are more likely to come in contact with each other than
                    people in other rings--for any reason.
- social_dist.json  Social distancing behavior for people. This should model actual behavior not
                    regulations. Extremely strict social distancing can be construed as quarantining.
- output            The output directory for the case: it is best to just use "output," the default.
)TAG";

inline constexpr std::string_view config_help = R"TAG(
Config Help

The keys, default values, and meaning of each entry in config.json:

{
  "days": 180,                      // Number of days the simulation runs
  "locale": 38015,                  // Cusip location for a city or county: Must match a row in geodata.csv
  "calendar_start": "2020-01-01",   // Day 1 of the simulation is set to this actual calendar date for seasonality and
                                       // to correspond to an actual epidemic.
  "dovax": false,                   // Adminster vaccines: true or false
  "do_social_distancing": false,    // Apply social distancing cases: true or false
  "do_rings": false,                // Assign ring membership and use ring contact rules: true or false
  "debug": false,                   // Print verbose debug diagnostics such as overflow warnings
  "rt_sim_interval": 50,            // Estimate Rt every n days.  Use 0 to never estimate Rt.
  "age_dist": [0.251, 0.271, 0.255, 0.184, 0.039],  // fraction of population each age group:  0-19, 20-29, 
                                                    // 40-59, 60-79, 80 and over
  "case_desc": "young: no distancing; old: rigorous; Moderna 50pct", // optional description
  // the following parameters are the names of the other input parameter files:
  "geodata":           "geodata.csv",
  "variants":          "variants.json",
  "social_params":     "socialparams.json",
  "seed":              "seed.json",
  "vaccines":          "vaccines.json",
  "vax_sched_dir":     "vaccine_100k",
  "rings":             "rings.json",
  "social_dist":       "soc_dist.json",
  "output":             "output"    // the output directory for the case
}
)TAG";

inline constexpr std::string_view seed_help = R"TAG(Seed Help
_________

seed.json controls scheduled changes to individual people, usually to introduce
initial infectious cases. Each seed case filters the population, then applies the
listed trait changes to a requested number of matching people. Use startofday to
choose whether the change happens before or after that day's simulation steps.

Template (JSONC):

// seed.json is an array; each object is one seeding event.
[
    {
        // Simulation day when this seed case runs.
        "triggerday": 1,
        // true applies before daily spread/progression; false applies at day end.
        "startofday": true,
        // All filter terms must match before a person can be selected.
        "filter": [
            // trait is an AgentView column or derived flag; val is its requested value.
            {"trait": "agegrp", "val": "Age20_39"}
        ],
        // change describes the trait updates to apply to selected people.
        "change": {
            // status=infectious must be paired with a real variant.
            "terms": [
                {"trait": "status",   "val": "infectious"},
                {"trait": "cond",     "val": "nil"},
                {"trait": "variant",  "val": "base"},
                {"trait": "duration", "val": 1}
            ],
            // Number of matching people to change.
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

inline constexpr std::string_view geodata_help = R"TAG(GeoData Help
____________

geodata.csv controls the locations available to a case. The config.json locale
must match a fips value in this file, and the matching row supplies population,
density, and indoor-season dates. You can add real or fictional locations as
long as the required columns remain present.

Template (commented CSV):

# geodata.csv is CSV, not JSON.
# fips is the locale identifier used by config.json "locale".
# pop and density control population size and density effects.
# indoor_st and indoor_end define the recurring indoor season for each locale.
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

inline constexpr std::string_view socialdistancing_help = R"TAG(Social Distancing Help
_______________________

soc_dist.json controls named social-distancing cases. Each case applies to a day
range and to either all age groups or the listed age groups, selecting a fraction
of eligible people to comply. While active, the case uses rescaled contact and
touch factors derived from socialparams.json.

Template (JSONC):

// soc_dist.json is an array; each object is one named distancing case.
[  
    {
        // Name registered for this social-distancing case.
        "name": "young",
        // First simulation day on which this case marks complying people.
        "startday": 50,
        // Day on which marks for this case are cleared; omit for no planned end.
        "endday": 100,
        // Fraction of eligible people who comply.
        "comply": 0.9,
        // Range used to rescale contact factors while this case is active.
        "contact_delta": [0.2, 1.0],
        // Range used to rescale touch probabilities while this case is active.
        "touch_delta": [0.18, 0.6],
        // Empty means all age groups; otherwise list age groups to include.
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

inline constexpr std::string_view socialparams_help = R"TAG(
Social Parameters Help
----------------------

The keys, default values, and meaning of each entry in socialparams.json:

{
  "gammashape": 1.0,      // Shape of the gamma distribution used to vary the number of contacts.
  "indoor_uplift": 1.1,   // Multiplies contacts and touch probability during the locale's indoor period.
  "contactfactors": {     // Multiplier for contacts made by an infectious person, by age group and condition.
    "age0_19": { "nil": 1.045, "mild": 1.045, "sick": 0.665, "severe": 0.475 },
    "age20_39": { "nil": 1.995, "mild": 1.9, "sick": 0.95, "severe": 0.57 },
    "age40_59": { "nil": 1.995, "mild": 1.9, "sick": 0.95, "severe": 0.57 },
    "age60_79": { "nil": 1.615, "mild": 1.52, "sick": 0.665, "severe": 0.475 },
    "age80_up": { "nil": 0.95, "mild": 0.855, "sick": 0.57, "severe": 0.475 }
  },
  "touchfactors": {      // Probability that a contacted person is touched, by age group and status or condition.
    "age0_19": { "unexposed": 0.55, "recovered": 0.55, "nil": 0.55, "mild": 0.55, "sick": 0.28, "severe": 0.18 },
    "age20_39": { "unexposed": 0.63, "recovered": 0.63, "nil": 0.63, "mild": 0.62, "sick": 0.35, "severe": 0.18 },
    "age40_59": { "unexposed": 0.61, "recovered": 0.61, "nil": 0.61, "mild": 0.58, "sick": 0.3, "severe": 0.18 },
    "age60_79": { "unexposed": 0.41, "recovered": 0.41, "nil": 0.41, "mild": 0.41, "sick": 0.18, "severe": 0.18 },
    "age80_up": { "unexposed": 0.35, "recovered": 0.35, "nil": 0.35, "mild": 0.28, "sick": 0.18, "severe": 0.18 }
  }
}
)TAG";

inline constexpr std::string_view vaccine_help = R"TAG(Vaccines Help
_____________

vaccines.json controls the available vaccine products and their effects by
variant and shot stage. Timing fields describe dose spacing, booster delay, and
how many days it takes to reach full effect after a shot. The variant keys under
infectfactor and effectiveness must stay aligned with variants.json.

Template (JSONC):


// vaccines.json is an object keyed by vaccine product name.
{
    // Each vaccine entry uses the same fields.
    "Pfizer": {
        // Days for vaccine-derived protection to halve.
        "halflife": 360,
        // Number of shots in the primary series.
        "reqdshots": 2,
        // Days after first shot before a second shot.
        "delay2ndshot": 21,
        // Days after full vaccination before booster eligibility.
        "delaybooster": 160,
        // Days after a shot to reach full effect.
        "full_effect_days": 14,
        // Initial fraction of the eventual shot effect.
        "day1_effect": 0.65,
        // Relative infection risk by infecting variant.
        "infectfactor": {
            "base": 0.9,
            "alpha": 0.9,
            "delta": 0.85,
            "omicron_ba1": 0.75,
            "omicron_ba2": 0.75,
            "omicron_ba4_5": 0.75
        },
        // Protection values by shot stage and variant.
        "effectiveness": {
            // First-shot protection.
            "first": {
                "base": 0.9,
                "alpha": 0.9,
                "delta": 0.7,
                "omicron_ba1": 0.65,
                "omicron_ba2": 0.65,
                "omicron_ba4_5": 0.65
            },
            // Full primary-series protection.
            "full": {
                "base": 0.94,
                "alpha": 0.94,
                "delta": 0.8,
                "omicron_ba1": 0.85,
                "omicron_ba2": 0.85,
                "omicron_ba4_5": 0.85
            },
            // Booster protection.
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

inline constexpr std::string_view variants_help = R"TAG(Variants Help
_____________

Each variant of a pathogen has characteristics that affect the spread of the disease
and progression through stages of the illness after being infected. Each variant has three
categories of parameters:
  - spread:       probabilities of an infection for the "sender" and the receiver
  - immunity:     reduction of future infections after having been infected, based on
                    the variant of the subsequent infection
  - progression:  probabilities of changing conditions during the illness. the
                  conditions are:
                  - nil:    carrier of the pathogen but asymptomatic
                  - mild:   some symptoms that don't affect daily activities
                  - sick:   evident symptoms that significantly reduce daily activities
                  - severe: symptoms that force a person to be bedridden, are debilitating, 
                            potentially requiring hospitalization and possibly life threatening

The progression-tree transition vectors determine how an infected person's
condition changes over the course of infection. There is a probability for each outcome:
  - recover:    no longer infected
  - nil
  - mild
  - sick
  - severe
  - dead
The progressions are assessed in the simulation a number of days after initial infection.
The progressions depend on the age group of the infected person.

This is a lot of parameters.  To simplify providing parameters for many variants, a basemultiplier may 
be used as a multiplicative factor for the parameters of the base variant.

Template (JSONC):

{
    // "base" is the variant's name and must be the first variant. Later variants can inherit its data.
    "base"         : {
        "spread"             : {
            // sendrisk: For each day after initial infection, probability of infected person sending disease
            //   to a victim.
            "sendrisk"      : [
                0   , 0.3 , 0.65, 0.75, 0.85, 0.85, 0.75, 0.7 , 0.65, 0.6 , 0.5 , 0.2 , 0.1 , 0.1 , 0.1 , 0.05, 0.05,
                0   , 0   , 0   , 0   , 0   , 0   , 0   , 0
            ],
            // recvrisk: probability of becoming infected by age group age0_19 through age80_up.
            "recvrisk"      : [0.1, 0.39, 0.44, 0.54, 0.56],
            "basemultiplier": 1
        },
        "immunity"           : {
            // reduction of becoming infected by any variant after recovering from this variant
            "recovery_immunity": {
                "base"         : 0.8,
                "alpha"        : 0.8,
                "delta"        : 0.6,
                "omicron_ba1"  : 0.6,
                "omicron_ba2"  : 0.6,
                "omicron_ba4_5": 0.4
            },
            "immunehalflife"   : 360  // immunity effect declines over time
        },
        "progression_tree"   : {
            // Each row is [recover, nil, mild, sick, severe, dead].
            "age0_19" : {
                "5" : {     // 5th day of being infected
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
                "25": {     // last day of being infected
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
        "progression_factors": {   // 
            "riskadjust": [],
            "vaxhalflifeadjust": {"JnJ": 0.5, "Pfizer": 0.8, "Moderna": 0.9}
        }
    },
    "alpha"        : {
        // Empty risk arrays inherit the base values, scaled by basemultiplier.
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
        // null inherits the base progression tree, then applies riskadjust.
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
                0    , 0.445, 0.975, 1.125, 1.275, 1.275, 1.125, 1.05 , 0.975, 0.9  , 0.75 , 0.5  , 0.1  , 0.05,
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
                0    , 0.545, 1.175, 1.325, 1.475, 1.475, 1.325, 1.25 , 1.075, 0.9  , 0.75 , 0.5  , 0.1  , 0.05,
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
                0    , 0.545, 1.175, 1.325, 1.475, 1.475, 1.325, 1.25 , 1.075, 0.9  , 0.75 , 0.5  , 0.1  , 0.05,
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

inline constexpr std::string_view vaccineschedule_help = R"TAG(Vaccination Schedule Help
_________________________

The vaccine schedule directory controls when doses are available and which age
groups receive them. Each JSON file in the directory is one schedule, with a mix
of vaccine products, a simulation-day range, a target percentage, and a delivery
pattern. The scaffold creates two schedules under the default vaccine_100k
directory.

Template loc38015_old.json (JSONC):

// Each file in the vaccine schedule directory is one schedule.
{
// Vaccine products to distribute in this schedule.
"vaxesincluded": {
    "Pfizer": {
        // Fraction of scheduled doses assigned to this product; mixes must sum to 1.0.
        "mix": 0.5,
        // Available doses for this product in this schedule.
        "starting_doses": 40000,
        // Fraction of eligible people targeted for a second shot.
        "pct2ndshot": 0.9,
        // Fraction of eligible people targeted for a booster.
        "pctboost": 0.6,
        // Products to try when the preferred product is unavailable.
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
// Inclusive simulation-day range over which this schedule operates.
"dayrange": [
    350,
    700
],
// Target fraction of the filtered group to vaccinate.
"targetpct": 0.95,
// Age groups targeted by this schedule.
"filtervec": [
    "age80_up",
    "age60_79"
],
// Shot mode used by the vaccination logic.
"shotmode": "all",
// Relative daily distribution pattern within the schedule window.
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
// Reserved for future/custom spread functions; null uses the built-in pattern.
"spreadfunc": null
}

Template loc38015_young.json (JSONC):

// A second schedule can target a different age group and dose mix.
{
  // Vaccine products to distribute in this schedule.
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
  // Inclusive simulation-day range over which this schedule operates.
  "dayrange": [
      350,
      700
  ],
  // Target fraction of the filtered group to vaccinate.
  "targetpct": 0.65,
  // Age groups targeted by this schedule.
  "filtervec": [
      "age0_19",
      "age20_39",
      "age40_59"
  ],
  // Shot mode used by the vaccination logic.
  "shotmode": "all",
  // Relative daily distribution pattern within the schedule window.
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
  // Reserved for future/custom spread functions; null uses the built-in pattern.
  "spreadfunc": null
}
)TAG";

inline constexpr std::string_view gettingstarted_help = R"TAG(Getting Started Help
____________________

Start by creating a project directory or a standalone case directory, then edit
the generated files in the case input directory. A new case includes templates
for config.json, seed.json, variants.json, socialparams.json, optional vaccine
and social-distancing files, and an output directory for run results. The usual
workflow is: create a case, edit parameters, run the case, then inspect output.

Typical commands:

  epi_sim --set-project-dir ~/epi-project
  epi_sim --init-case case-1
  epi_sim --run-case case-1

For a standalone case directory:

  epi_sim --setup-dir ~/epi-case
  epi_sim --use-dir ~/epi-case
)TAG";

// help_map for all topics
struct HelpTopic {
  std::string_view key;
  std::string_view text;
};

// key appears in the help menu
// text are the help topic strings above
inline constexpr auto help_map = std::to_array<HelpTopic>({
    {"overview", overview_help},
    {"gettingstarted", gettingstarted_help},
    {"cases", project_cases_help},
    {"project", project_cases_help},
    {"parameters", parameters_help},
    {"socialparams", socialparams_help},
    {"config", config_help},
    {"seed", seed_help},
    {"vaccines", vaccine_help},
    {"variants", variants_help},
    {"geodata", geodata_help},
    {"socialdistancing", socialdistancing_help},
    {"vaccinationschedule", vaccineschedule_help},
  });


}
