#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

void show_help() {
  fmt::println("You must run epi_sim with one of the following flags and corresponding flag values:");
  fmt::println("-----------------------------------------------------------------------------------");
  fmt::println("  --set-project-dir <path>");
  fmt::println("  --show-project-dir");
  fmt::println("  --init-case <label>");
  fmt::println("  --use-case <label>");
  fmt::println("  --setup-dir <path>");
  fmt::println("  --use-dir <path>");
  fmt::println("  --help");
}
