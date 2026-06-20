#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include "show_help.h"
#include <string_view>
#include <array>

// 
// provide command line help in a terminal session
//

namespace {
// help text to display
constexpr std::string_view cases_help = R"TAG(This is the 
cases help.
)TAG";

constexpr std::string_view parameters_help = R"TAG(Everything about parameters.
)TAG";

constexpr std::string_view socialparameters_help = R"TAG(All about social parameters.
)TAG";

constexpr std::string_view config_help = R"TAG(This is the config help.
)TAG";

constexpr std::string_view seed_help = R"TAG(This is the seed help.
)TAG";

constexpr std::string_view geodata_help = R"TAG(This is the geodata help.
)TAG";

constexpr std::string_view socialdistancing_help = R"TAG(This is about socialdistancing.
)TAG";

constexpr std::string_view vaccine_help = R"TAG(This is all about vaccines.
)TAG";

constexpr std::string_view variants_help = R"TAG(This is all about variants.
)TAG";

constexpr std::string_view vaccineschedule_help = R"TAG(This is about the vaccination schedule.
)TAG";

constexpr std::string_view overview_help = R"TAG(
Overview
-----------
epi_sim is an agent-based epidemic simulation model.

Agent-based means each individual in a populution is tracked during the simulation. The
simulation is controlled by sets of parameters that determine the social pattern of spreading
of the pathogen; the transmissibility of the pathogen; the progression through stages of
the disease of those who become infected; the effectiveness of available vaccines;  schedules for
administering vaccine doses; the population size, population density and seasonality of people spending 
more time indoors; seeding the initial arrival of carriers of the pathogen; and 
social distancing patterns among people whether imposed or voluntary. "

Most of the parameters are entered in .json files (and .csv for the population data). The application
will create template files with valid entries for all the parameters. You should create an overall 
project directory to hold various cases. Each case will start with all of the needed parameter inputs.
You can edit these files to create the epidemic conditions you wish to simulate. Or you can create one
case at a time in a directory of your choice.  

To learn more:
    epi_sim --help
    epi_sim --help cases
    epi_sim --help parameters
    epi_sim --help gettingstarted
)TAG";

constexpr std::string_view gettingstarted_help = R"TAG(This the recommended way to start using epi_sim.
)TAG";

// help_map for all topics
using HelpTopic = std::pair<std::string_view, std::string_view>;

constexpr auto help_map = std::to_array<HelpTopic>({
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

  std::string_view get_or(std::string_view topic, std::string_view missing_reply) {
  for (const auto& [key, text] : help_map) {
    if (key == topic) return text;
  }
  return missing_reply;
}
}

void show_topics() {
  fmt::println("Available help topics:");
  for (auto [topic, text] : help_map) {
    println("  {}", topic);
  }
}

void show_help() {
  fmt::println("You must run epi_sim with one of the following flags and corresponding flag values:");
  fmt::println("-----------------------------------------------------------------------------------");
  fmt::println("  --set-project-dir <path in user's home directory>");
  fmt::println("  --show-project-dir");
  fmt::println("  --init-case <case label>");
  fmt::println("  --use-case <case label>");
  fmt::println("  --show-cases");
  fmt::println("  --setup-dir <path in user's home directory>");
  fmt::println("  --use-dir <path>");
  fmt::println("  --r0-sim <path or case label");
  fmt::println("  --help");
  fmt::println("\n");
  fmt::println("For more specific help, run epi_sim --help topics.");

  show_topics();
}


void show_help(std::string_view topic) {
  std::string_view txt {get_or(topic, "Topic not found")};
  fmt::println("{}", txt);
  if (txt == "Topic not found") show_topics();

}
