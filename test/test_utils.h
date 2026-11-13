// test_utils.h -- minimal test harness
//
// - run_test(name, fn): runs one test; REQUIRE aborts and any exception thrown by
//   source code are recorded as failures of that test and the run continues.
// - CHECK / CHECK_MSG / CHECK_EQ / CHECK_NEAR / CHECK_THROWS print the test name,
//   location, actual values, and an optional hint.
// - KNOWN_BUG(id, expr, ...) records checks for known, not-yet-fixed source defects in a
//   separate list that does not fail the exit code. A passing KNOWN_BUG is reported as
//   "now passes -- remove the marker".
// - a crash handler prints the active group/test before the process dies.
#pragma once

#include <csignal>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace tst {

struct Abort : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct KnownBug {
  std::string id;
  std::string where;
  std::string detail;
  bool fixed;
};

struct GroupTally {
  std::string name;
  int pass{0};
  int fail{0};
  int tests{0};
  int failed_tests{0};
};

struct State {
  std::string group{"(none)"};
  std::string test{"(none)"};
  int pass{0};
  int fail{0};
  std::vector<std::string> failed_tests;
  std::vector<KnownBug> known;
  std::vector<GroupTally> groups;
};

inline State& state() {
  static State s;
  return s;
}

// fixed buffer read by the crash handler (no allocation in the handler)
inline char* crash_label() {
  static char buf[256] = "(none)";
  return buf;
}

inline void set_crash_label() {
  const auto s = fmt::format("{}/{}", state().group, state().test);
  std::strncpy(crash_label(), s.c_str(), 255);
  crash_label()[255] = '\0';
}

inline void crash_handler(int sig) {
  const char* msg1 = "\n[CRASH] fatal signal in test ";
  ::write(STDERR_FILENO, msg1, std::strlen(msg1));
  ::write(STDERR_FILENO, crash_label(), std::strlen(crash_label()));
  const char* msg2 = " -- the run stopped here; results above are incomplete.\n";
  ::write(STDERR_FILENO, msg2, std::strlen(msg2));
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}

inline void install_crash_handler() {
  for (int sig : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT}) std::signal(sig, crash_handler);
}

// render a value for diagnostics: trait wrappers via show(), else fmt, else numeric
template <typename T>
std::string show(const T& v) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<U, bool>) {
    return v ? "true" : "false";
  } else if constexpr (requires { v.dump(); }) {  // json: never use its implicit conversions
    return v.dump();
  } else if constexpr (requires { v.show(); static_cast<int>(v); }) {
    return fmt::format("{}({})", v.show(), static_cast<int>(v));
  } else if constexpr (requires { v.show(); }) {
    return std::string{v.show()};
  } else if constexpr (fmt::is_formattable<U>::value) {
    return fmt::format("{}", v);
  } else if constexpr (std::is_convertible_v<U, long long>) {
    return fmt::format("{}", static_cast<long long>(v));
  } else {
    return "<unprintable>";
  }
}

inline bool check(bool ok, std::string_view expr, const char* file, int line,
                  std::string_view detail = {}) {
  auto& s = state();
  if (ok) {
    ++s.pass;
    return true;
  }
  ++s.fail;
  fmt::print(stderr, "[FAIL] {}/{}: {}\n       {}:{}\n", s.group, s.test, expr, file, line);
  if (!detail.empty()) fmt::print(stderr, "       {}\n", detail);
  return false;
}

inline void known_bug(std::string_view id, bool ok, std::string_view expr, const char* file,
                      int line, std::string_view detail = {}) {
  auto& s = state();
  const std::string where = fmt::format("{}/{} ({}:{})", s.group, s.test, file, line);
  for (const auto& k : s.known)
    if (k.id == id && k.where == where) return;  // report each site once
  s.known.push_back({std::string(id), where,
                     fmt::format("{}{}{}", expr, detail.empty() ? "" : " -- ", detail), ok});
  if (!ok) fmt::print(stderr, "[KNOWN {}] {}: {}\n", id, where, detail.empty() ? expr : detail);
}

// Runs one test function; never lets an exception escape.
template <typename F>
void run_test(std::string_view name, F&& fn) {
  auto& s = state();
  s.test = std::string(name);
  set_crash_label();
  const int fail_before = s.fail;
  try {
    fn();
  } catch (const Abort& e) {
    fmt::print(stderr, "       (test '{}' stopped at a failed REQUIRE)\n", name);
  } catch (const std::exception& e) {
    ++s.fail;
    fmt::print(stderr, "[ERROR] {}/{}: uncaught exception: {}\n", s.group, name, e.what());
  } catch (...) {
    ++s.fail;
    fmt::print(stderr, "[ERROR] {}/{}: uncaught non-standard exception\n", s.group, name);
  }
  if (!s.groups.empty()) ++s.groups.back().tests;
  if (s.fail > fail_before) {
    s.failed_tests.push_back(fmt::format("{}/{}", s.group, name));
    if (!s.groups.empty()) ++s.groups.back().failed_tests;
  }
  s.test = "(none)";
  set_crash_label();
}

inline void begin_group(std::string_view name) {
  auto& s = state();
  s.group = std::string(name);
  s.groups.push_back({std::string(name), s.pass, s.fail, 0, 0});
  set_crash_label();
  fmt::println("\n=== {} ===", name);
}

inline void end_group() {
  auto& s = state();
  if (s.groups.empty()) return;
  auto& g = s.groups.back();
  g.pass = s.pass - g.pass;
  g.fail = s.fail - g.fail;
  fmt::println("--- {}: {} tests, {} checks, {} failed checks{}", g.name, g.tests,
               g.pass + g.fail, g.fail,
               g.failed_tests ? fmt::format(" in {} test(s)", g.failed_tests) : "");
  s.group = "(none)";
}

inline int summary() {
  auto& s = state();
  fmt::println("\n================ test summary ================");
  for (const auto& g : s.groups)
    fmt::println("  {:<18} {:>3} tests {:>6} checks {:>4} failed", g.name, g.tests, g.pass + g.fail,
                 g.fail);
  fmt::println("  total: {} checks, {} passed, {} failed", s.pass + s.fail, s.pass, s.fail);
  if (!s.failed_tests.empty()) {
    fmt::println("\nFailed tests:");
    for (const auto& t : s.failed_tests) fmt::println("  - {}", t);
  }
  bool any_open = false, any_fixed = false;
  for (const auto& k : s.known) (k.fixed ? any_fixed : any_open) = true;
  if (any_open) {
    fmt::println("\nKnown source bugs still present (not counted as failures; see "
                 "design/test_suite_revision.md):");
    for (const auto& k : s.known)
      if (!k.fixed) fmt::println("  - [{}] {}\n      {}", k.id, k.where, k.detail);
  }
  if (any_fixed) {
    fmt::println("\nKnown-bug checks that now PASS -- the bug looks fixed; change KNOWN_BUG to CHECK:");
    for (const auto& k : s.known)
      if (k.fixed) fmt::println("  - [{}] {}", k.id, k.where);
  }
  fmt::println("==============================================");
  return s.fail > 0 ? 1 : 0;
}

}  // namespace tst

#define CHECK(expr) ::tst::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

#define CHECK_MSG(expr, ...) \
  ::tst::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__, ::fmt::format(__VA_ARGS__))

#define CHECK_EQ(a, b)                                                                     \
  do {                                                                                     \
    const auto tst_a_ = (a); /* copies: no dangling refs into temporaries */             \
    const auto tst_b_ = (b);                                                               \
    ::tst::check(tst_a_ == tst_b_, #a " == " #b, __FILE__, __LINE__,                       \
                 ::fmt::format("left = {}, right = {}", ::tst::show(tst_a_), ::tst::show(tst_b_))); \
  } while (0)

#define CHECK_NEAR(a, b, tol)                                                              \
  do {                                                                                     \
    const double tst_a_ = static_cast<double>(a);                                          \
    const double tst_b_ = static_cast<double>(b);                                          \
    ::tst::check(std::abs(tst_a_ - tst_b_) <= (tol), #a " ~= " #b, __FILE__, __LINE__,     \
                 ::fmt::format("left = {:.6g}, right = {:.6g}, |diff| = {:.3g} > tol {}",   \
                               tst_a_, tst_b_, std::abs(tst_a_ - tst_b_), (tol)));         \
  } while (0)

// CHECK_THROWS(ExceptionType, statement, "substring of what()" or "")
#define CHECK_THROWS(ExType, stmt, substr)                                                 \
  do {                                                                                     \
    std::string tst_msg_;                                                                  \
    bool tst_threw_ = false, tst_other_ = false;                                           \
    try { stmt; } catch (const ExType& e) { tst_threw_ = true; tst_msg_ = e.what(); }      \
    catch (const std::exception& e) { tst_other_ = true; tst_msg_ = e.what(); }            \
    const std::string tst_sub_{substr};                                                    \
    ::tst::check(tst_threw_ && tst_msg_.find(tst_sub_) != std::string::npos,               \
                 "throws " #ExType " from: " #stmt, __FILE__, __LINE__,                    \
                 tst_threw_ ? ::fmt::format("message '{}' lacks '{}'", tst_msg_, tst_sub_)  \
                 : tst_other_ ? ::fmt::format("threw a different type: '{}'", tst_msg_)     \
                              : std::string("did not throw"));                             \
  } while (0)

#define REQUIRE(expr)                                                                      \
  do {                                                                                     \
    if (!::tst::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__, "REQUIRE: test aborted")) \
      throw ::tst::Abort("REQUIRE failed: " #expr);                                        \
  } while (0)

#define KNOWN_BUG(id, expr, ...) \
  ::tst::known_bug(id, static_cast<bool>(expr), #expr, __FILE__, __LINE__, ::fmt::format(__VA_ARGS__))

// compatibility with the old harness
inline int test_failure_count() { return ::tst::state().fail; }
inline void test_summary() { (void)::tst::summary(); }
