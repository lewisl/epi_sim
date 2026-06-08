#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

void show_help() {
  fmt::println("You must run epi_sim with one of the following flags and corresponding flag values:");
  fmt::println("-----------------------------------------------------------------------------------");
  fmt::println("  --set-project-dir <path>");
  fmt::println("  --help");
}