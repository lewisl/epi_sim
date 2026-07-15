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

inline constexpr std::string_view cases_help = "This is the cases help.\n";

inline constexpr std::string_view parameters_help = R"TAG(
Parameters Help

The following parameter files are placed in the input directory of a case directory:
- config.json     Essential parameters that drive the simulation, including:
                    - length in days;
                    - the calendar date your epidemic begins;
                    - the age distribution of your population by 20 year cohorts;
                    - a brief description
                    - the file names for the other parameter files (best to 
                      just use the provided defaults);
- geodata.csv     The population of several cities. Feel free to create one or more "fake" cities.
- variants        Variants of a virus pathogen and infectiousness and recovery 
                  characteristics of each variant.
- seed            Number and characteristics of people who start the epidemic. You can have more than
                  one seeding event, especially to prolong the epidemic across many months.
- vaccines        Name and efficacy of available vaccines.
- vax_sched_dir   Explicit distribution of vaccines by time period and total available doses. The 
                  directory includes multiple schedules for different age groups and timing.
- rings           Contact groups for infected people spreading the disease. "Ring" simply means
                  people within a ring are more likely to come in contact with each other than
                  people in other rings--for any reason.
- social_dist     Social distancing behavior for people. This should model actual behavior not
                  regulations. Extremely strict social distancing can be construed as quarantining.
- output          The output directory for the case: it is best to just use "output," the default.
)TAG";

inline constexpr std::string_view socialparameters_help = R"TAG(All about social parameters.
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

inline constexpr std::string_view seed_help = R"TAG(This is the seed help.
)TAG";

inline constexpr std::string_view geodata_help = R"TAG(This is the geodata help.
)TAG";

inline constexpr std::string_view socialdistancing_help = R"TAG(This is about socialdistancing.
)TAG";

inline constexpr std::string_view vaccine_help = R"TAG(This is all about vaccines.
)TAG";

inline constexpr std::string_view variants_help = R"TAG(This is all about variants.
)TAG";

inline constexpr std::string_view vaccineschedule_help = R"TAG(This is about the vaccination schedule.
)TAG";

inline constexpr std::string_view gettingstarted_help = R"TAG(This the recommended way to start using epi_sim.
)TAG";

// help_map for all topics
struct HelpTopic {
  std::string_view key;
  std::string_view text;
};

inline constexpr auto help_map = std::to_array<HelpTopic>({
    {"overview", overview_help},
    {"gettingstarted", gettingstarted_help},
    {"cases", cases_help},
    {"parameters", parameters_help},
    {"socialparameters", socialparameters_help},
    {"config", config_help},
    {"seed", seed_help},
    {"vaccines", vaccine_help},
    {"variants", variants_help},
    {"geodata", geodata_help},
    {"socialdistancing", socialdistancing_help},
    {"vaccinationschedule", vaccineschedule_help},
  });


}
