#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include "show_help.h"
#include <string_view>

// 
// provide command line help in a terminal session
//

namespace {

  std::string_view get_or(std::string_view topic, std::string_view missing_reply) {
  for (const auto& [key, text] : hlptxt::help_map) {
    if (key == topic) return text;
  }
  return missing_reply;
}
}

void show_topics() {
  fmt::println("Available help topics:");
  for (auto [topic, text] : hlptxt::help_map) {
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
