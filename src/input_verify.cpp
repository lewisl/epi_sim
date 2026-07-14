#include "input_verify.h"

#include <fmt/format.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>

namespace fs = std::filesystem;



namespace input_verify_detail {

//
// re-usable detail and value tests
//  
namespace {

// The five real age-group keys shared by rings / socialparams / progression.
constexpr std::array<const char*, 5> AGE_KEYS = {
    "age0_19", "age20_39", "age40_59", "age60_79", "age80_up"};

// Lightweight YYYY-MM-DD shape check (parse_date does not reject bad input).
bool looks_like_iso_date(const std::string& s) {
  if (s.size() != 10) return false;
  for (size_t i = 0; i < 10; ++i) {
    const char c = s[i];
    if (i == 4 || i == 7) {
      if (c != '-') return false;
    } else if (c < '0' || c > '9') {
      return false;
    }
  }
  const int month = (s[5] - '0') * 10 + (s[6] - '0');
  const int day = (s[8] - '0') * 10 + (s[9] - '0');
  return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

// Records a missing-key error and returns false when absent.
bool has_key(const json& j, const char* key, std::string_view ctx, Errors& e) {
  if (!j.is_object() || !j.contains(key)) {
    e.add(fmt::format("{}: missing required key '{}'.", ctx, key));
    return false;
  }
  return true;
}

void need_string(const json& j, const char* key, std::string_view ctx, Errors& e) {
  if (!has_key(j, key, ctx, e)) return;
  if (!j[key].is_string())
    e.add(fmt::format("{}: key '{}' must be a string.", ctx, key));
}

void need_int(const json& j, const char* key, std::string_view ctx, Errors& e) {
  if (!has_key(j, key, ctx, e)) return;
  if (!j[key].is_number_integer())
    e.add(fmt::format("{}: key '{}' must be an integer.", ctx, key));
}

void need_number(const json& j, const char* key, std::string_view ctx, Errors& e) {
  if (!has_key(j, key, ctx, e)) return;
  if (!j[key].is_number())
    e.add(fmt::format("{}: key '{}' must be a number.", ctx, key));
}

void need_bool(const json& j, const char* key, std::string_view ctx, Errors& e) {
  if (!has_key(j, key, ctx, e)) return;
  if (!j[key].is_boolean())
    e.add(fmt::format("{}: key '{}' must be a boolean.", ctx, key));
}

void need_array(const json& j, const char* key, std::string_view ctx, Errors& e) {
  if (!has_key(j, key, ctx, e)) return;
  if (!j[key].is_array())
    e.add(fmt::format("{}: key '{}' must be an array.", ctx, key));
}

void need_object(const json& j, const char* key, std::string_view ctx, Errors& e) {
  if (!has_key(j, key, ctx, e)) return;
  if (!j[key].is_object())
    e.add(fmt::format("{}: key '{}' must be an object.", ctx, key));
}

// Safe local JSON load: on open/parse failure, records an error and returns
// nullopt instead of throwing, so verify never itself throws on bad input.
std::optional<json> try_load_json(const fs::path& p, std::string_view label, Errors& e) {
  std::ifstream in(p);
  if (!in) {
    e.add(fmt::format("{}: cannot open file '{}'.", label, p.string()));
    return std::nullopt;
  }
  try {
    return json::parse(in, nullptr, true, true);  // allow_exceptions, ignore_comments
  } catch (const std::exception& ex) {
    e.add(fmt::format("{}: '{}' is not valid JSON ({}).", label, p.string(), ex.what()));
    return std::nullopt;
  }
}

}  // namespace

bool near_one(double sum) { return std::abs(sum - 1.0) < 1e-6; }

bool write_error_log(const fs::path& log_path, std::string_view report) {
  std::ofstream log(log_path, std::ios::trunc);
  if (!log) return false;
  log << report;
  log.close();
  return static_cast<bool>(log);
}


//
//   check config.json
//
void check_config(const json& cfg, Errors& e, bool& dovax, bool& do_rings,
                  bool& do_social_distancing, int& locale) {
  constexpr std::string_view ctx = "config.json";
  dovax = false;
  do_rings = false;
  do_social_distancing = false;
  locale = 0;

  if (!cfg.is_object()) {
    e.add(fmt::format("{}: expected a top-level JSON object.", ctx));
    return;
  }

  auto read_flag = [&](const char* k) {
    return cfg.contains(k) && cfg[k].is_boolean() && cfg[k].get<bool>();
  };

  need_int(cfg, "days", ctx, e);
  if (cfg.contains("days") && cfg["days"].is_number_integer() && cfg["days"].get<int>() <= 0)
    e.add(fmt::format("{}: 'days' must be > 0 (got {}).", ctx, cfg["days"].get<int>()));

  need_int(cfg, "locale", ctx, e);
  if (cfg.contains("locale") && cfg["locale"].is_number_integer())
    locale = cfg["locale"].get<int>();

  need_string(cfg, "calendar_start", ctx, e);
  if (cfg.contains("calendar_start") && cfg["calendar_start"].is_string() &&
      !looks_like_iso_date(cfg["calendar_start"].get<std::string>()))
    e.add(fmt::format("{}: 'calendar_start' must be a YYYY-MM-DD date (got '{}').",
                      ctx, cfg["calendar_start"].get<std::string>()));

  need_bool(cfg, "dovax", ctx, e);
  dovax = read_flag("dovax");

  need_int(cfg, "rt_sim_interval", ctx, e);
  if (cfg.contains("rt_sim_interval") && cfg["rt_sim_interval"].is_number_integer() &&
      cfg["rt_sim_interval"].get<int>() < 0)
    e.add(fmt::format("{}: 'rt_sim_interval' must be >= 0 (got {}).", ctx,
                      cfg["rt_sim_interval"].get<int>()));

  // age_dist: array of exactly 5 numbers summing to ~1.0 (current null-crash source).
  need_array(cfg, "age_dist", ctx, e);
  if (cfg.contains("age_dist") && cfg["age_dist"].is_array()) {
    const auto& ad = cfg["age_dist"];
    if (ad.size() != 5) {
      e.add(fmt::format("{}: 'age_dist' must have exactly 5 entries (got {}).", ctx, ad.size()));
    } else {
      double sum = 0.0;
      bool all_num = true;
      for (const auto& v : ad) {
        if (!v.is_number()) all_num = false;
        else sum += v.get<double>();
      }
      if (!all_num) e.add(fmt::format("{}: 'age_dist' entries must all be numbers.", ctx));
      else if (!near_one(sum))
        e.add(fmt::format("{}: 'age_dist' must sum to 1.0 (got {}).", ctx, sum));
    }
  }

  need_string(cfg, "geodata", ctx, e);
  need_string(cfg, "variants", ctx, e);
  need_string(cfg, "social_params", ctx, e);
  need_string(cfg, "seed", ctx, e);

  // Optional-with-default booleans; only type-checked when present.
  for (const char* k : {"do_social_distancing", "do_rings", "debug"})
    if (cfg.contains(k)) need_bool(cfg, k, ctx, e);
  do_social_distancing = read_flag("do_social_distancing");
  do_rings = read_flag("do_rings");

  // Conditional filename keys, required only when their feature is enabled.
  if (dovax) {
    need_string(cfg, "vaccines", ctx, e);
    need_string(cfg, "vax_sched_dir", ctx, e);
  }
  if (do_rings) need_string(cfg, "rings", ctx, e);
  if (do_social_distancing) need_string(cfg, "social_dist", ctx, e);
}

//
// check variants.json
//
void check_variants(const json& j, Errors& e) {
  constexpr std::string_view ctx = "variants.json";
  if (!j.is_object() || j.empty()) {
    e.add(fmt::format("{}: expected a non-empty object of variants.", ctx));
    return;
  }
  // Primary/first variant must be named "base" (ordered_json preserves order).
  const std::string first = j.begin().key();
  if (first != "base")
    e.add(fmt::format("{}: the first variant must be named 'base' (got '{}').", ctx, first));

  for (const auto& [name, body] : j.items()) {
    const std::string vctx = fmt::format("variants.json variant '{}'", name);
    if (!body.is_object()) {
      e.add(fmt::format("{}: must be an object.", vctx));
      continue;
    }
    need_object(body, "spread", vctx, e);
    need_object(body, "immunity", vctx, e);
    need_object(body, "progression_factors", vctx, e);

    if (!body.contains("progression_tree")) {
      e.add(fmt::format("{}: missing required key 'progression_tree'.", vctx));
      continue;
    }
    const auto& tree = body["progression_tree"];
    if (!tree.is_null() && !tree.is_object()) {
      e.add(fmt::format("{}: 'progression_tree' must be an object or null.", vctx));
      continue;
    }
    if (!tree.is_object()) continue;
    for (const auto& [age, breakdays] : tree.items()) {
      if (!breakdays.is_object()) continue;
      for (const auto& [day, conds] : breakdays.items()) {
        if (!conds.is_object()) continue;
        for (const auto& [cond, row] : conds.items()) {
          if (!row.is_array()) continue;
          if (row.size() != 6) {
            e.add(fmt::format("{} progression_tree age '{}' day '{}' condition '{}': "
                              "must have exactly 6 probabilities (got {}).",
                              vctx, age, day, cond, row.size()));
            continue;
          }
          double sum = 0.0;
          bool all_num = true;
          for (const auto& v : row) {
            if (!v.is_number()) all_num = false;
            else sum += v.get<double>();
          }
          if (!all_num)
            e.add(fmt::format("{} progression_tree age '{}' day '{}' condition '{}': "
                              "probabilities must all be numbers.", vctx, age, day, cond));
          else if (!near_one(sum))
            e.add(fmt::format("{} progression_tree age '{}' day '{}' condition '{}': "
                              "probabilities must sum to 1.0 (got {}).",
                              vctx, age, day, cond, sum));
        }
      }
    }
  }
}


//
// check vaccines.json
//
void check_vaccines(const json& j, Errors& e) {
  constexpr std::string_view ctx = "vaccines.json";
  if (!j.is_object() || j.empty()) {
    e.add(fmt::format("{}: expected a non-empty object of vaccines.", ctx));
    return;
  }
  for (const auto& [name, body] : j.items()) {
    const std::string vctx = fmt::format("vaccines.json vaccine '{}'", name);
    if (!body.is_object()) {
      e.add(fmt::format("{}: must be an object.", vctx));
      continue;
    }
    for (const char* k :
         {"halflife", "reqdshots", "delay2ndshot", "delaybooster", "full_effect_days"})
      need_int(body, k, vctx, e);
    need_number(body, "day1_effect", vctx, e);
    need_object(body, "infectfactor", vctx, e);
    need_object(body, "effectiveness", vctx, e);
  }
}


//
// check specific vax_sched schedules
//
void check_vax_sched(const json& j, std::string_view label, Errors& e) {
  const std::string ctx = fmt::format("vax schedule '{}'", label);
  if (!j.is_object()) {
    e.add(fmt::format("{}: expected a JSON object.", ctx));
    return;
  }
  need_object(j, "vaxesincluded", ctx, e);
  need_array(j, "dayrange", ctx, e);
  if (j.contains("dayrange") && j["dayrange"].is_array() && j["dayrange"].size() != 2)
    e.add(fmt::format("{}: 'dayrange' must have exactly 2 entries (got {}).",
                      ctx, j["dayrange"].size()));
  need_number(j, "targetpct", ctx, e);
  need_array(j, "filtervec", ctx, e);
  need_string(j, "shotmode", ctx, e);
  need_array(j, "pattern", ctx, e);

  // mix values across vaxesincluded must sum to ~1.0 (categorical_fast silently
  // falls back to brand index 0 otherwise).
  if (j.contains("vaxesincluded") && j["vaxesincluded"].is_object() &&
      !j["vaxesincluded"].empty()) {
    double mix_sum = 0.0;
    bool all_num = true;
    for (const auto& [vax, spec] : j["vaxesincluded"].items()) {
      if (!spec.is_object() || !spec.contains("mix") || !spec["mix"].is_number()) {
        all_num = false;
        continue;
      }
      mix_sum += spec["mix"].get<double>();
    }
    if (!all_num)
      e.add(fmt::format("{}: every vaxesincluded entry must have a numeric 'mix'.", ctx));
    else if (!near_one(mix_sum))
      e.add(fmt::format("{}: 'mix' values must sum to 1.0 (got {}).", ctx, mix_sum));
  }
}


//
// check socialparams.json
//
void check_socialparams(const json& j, Errors& e) {
  constexpr std::string_view ctx = "socialparams.json";
  if (!j.is_object()) {
    e.add(fmt::format("{}: expected a JSON object.", ctx));
    return;
  }
  need_number(j, "gammashape", ctx, e);
  need_number(j, "indoor_uplift", ctx, e);
  need_object(j, "contactfactors", ctx, e);
  need_object(j, "touchfactors", ctx, e);
}

//
// check seed.json
//
void check_seed(const json& j, Errors& e) {
  constexpr std::string_view ctx = "seed.json";
  if (!j.is_array()) {
    e.add(fmt::format("{}: expected a JSON array of seed cases.", ctx));
    return;
  }
  for (size_t i = 0; i < j.size(); ++i) {
    const auto& el = j[i];
    const std::string ectx = fmt::format("seed.json entry [{}]", i);
    if (!el.is_object()) {
      e.add(fmt::format("{}: must be an object.", ectx));
      continue;
    }
    need_int(el, "triggerday", ectx, e);
    need_bool(el, "startofday", ectx, e);
    need_array(el, "filter", ectx, e);
    need_object(el, "change", ectx, e);
  }
}


//
// check soc_dist.json
//
void check_soc_dist(const json& j, Errors& e) {
  constexpr std::string_view ctx = "soc_dist.json";
  if (!j.is_array()) {
    e.add(fmt::format("{}: expected a JSON array of social-distancing cases.", ctx));
    return;
  }
  for (size_t i = 0; i < j.size(); ++i) {
    const auto& el = j[i];
    const std::string ectx = fmt::format("soc_dist.json entry [{}]", i);
    if (!el.is_object()) {
      e.add(fmt::format("{}: must be an object.", ectx));
      continue;
    }
    need_string(el, "name", ectx, e);
    need_number(el, "startday", ectx, e);
    need_number(el, "endday", ectx, e);
    need_number(el, "comply", ectx, e);
    need_array(el, "contact_delta", ectx, e);
    if (el.contains("contact_delta") && el["contact_delta"].is_array() &&
        el["contact_delta"].size() != 2)
      e.add(fmt::format("{}: 'contact_delta' must have exactly 2 entries.", ectx));
    need_array(el, "touch_delta", ectx, e);
    if (el.contains("touch_delta") && el["touch_delta"].is_array() &&
        el["touch_delta"].size() != 2)
      e.add(fmt::format("{}: 'touch_delta' must have exactly 2 entries.", ectx));
    need_array(el, "include_ages", ectx, e);
  }
}


//
// check rings.json
//
void check_rings(const json& j, Errors& e) {
  constexpr std::string_view ctx = "rings.json";
  if (!j.is_object() || !j.contains("rings")) {
    e.add(fmt::format("{}: expected a top-level 'rings' key.", ctx));
    return;
  }
  const auto& arr = j["rings"];
  if (!arr.is_array() || arr.empty()) {
    e.add(fmt::format("{}: 'rings' must be a non-empty array.", ctx));
    return;
  }
  double pct_sum = 0.0;
  bool all_pct_num = true;
  for (size_t i = 0; i < arr.size(); ++i) {
    const auto& r = arr[i];
    const std::string rctx = fmt::format("rings.json ring [{}]", i);
    if (!r.is_object()) {
      e.add(fmt::format("{}: must be an object.", rctx));
      all_pct_num = false;
      continue;
    }
    need_string(r, "name", rctx, e);
    need_number(r, "pct_of_population", rctx, e);
    if (r.contains("pct_of_population") && r["pct_of_population"].is_number())
      pct_sum += r["pct_of_population"].get<double>();
    else
      all_pct_num = false;
    need_object(r, "out_ring_prob_by_agegrp", rctx, e);
    if (r.contains("out_ring_prob_by_agegrp") && r["out_ring_prob_by_agegrp"].is_object()) {
      for (const char* age : AGE_KEYS)
        if (!r["out_ring_prob_by_agegrp"].contains(age))
          e.add(fmt::format("{}: 'out_ring_prob_by_agegrp' missing agegrp '{}'.", rctx, age));
    }
  }
  if (all_pct_num && !near_one(pct_sum))
    e.add(fmt::format("{}: 'pct_of_population' values must sum to 1.0 (got {}).", ctx, pct_sum));
}


//
// check geodata.csv
//
void check_geodata_csv(const fs::path& p, int locale, Errors& e) {
  constexpr std::string_view ctx = "geodata.csv";
  std::ifstream in(p);
  if (!in) {
    e.add(fmt::format("{}: cannot open file '{}'.", ctx, p.string()));
    return;
  }
  std::string header;
  if (!std::getline(in, header)) {
    e.add(fmt::format("{}: file is empty.", ctx));
    return;
  }
  auto split = [](const std::string& line) {
    std::vector<std::string> cols;
    std::string cell;
    std::istringstream ss(line);
    while (std::getline(ss, cell, ',')) {
      const size_t b = cell.find_first_not_of(" \t\r\n");
      const size_t en = cell.find_last_not_of(" \t\r\n");
      cols.push_back(b == std::string::npos ? "" : cell.substr(b, en - b + 1));
    }
    return cols;
  };
  const std::vector<std::string> cols = split(header);
  constexpr std::array<const char*, 10> required = {
      "fips", "county", "city", "state", "sizecat",
      "pop", "density", "anchor", "indoor_st", "indoor_end"};
  int fips_idx = -1;
  for (const char* col : required) {
    const auto it = std::find(cols.begin(), cols.end(), col);
    if (it == cols.end())
      e.add(fmt::format("{}: header missing required column '{}'.", ctx, col));
    else if (std::string(col) == "fips")
      fips_idx = static_cast<int>(std::distance(cols.begin(), it));
  }
  if (fips_idx < 0) return;  // cannot match locale without a fips column

  bool found = false;
  std::string row;
  while (std::getline(in, row)) {
    if (row.find_first_not_of(" \t\r\n") == std::string::npos) continue;
    const std::vector<std::string> vals = split(row);
    if (static_cast<int>(vals.size()) <= fips_idx) continue;
    try {
      if (std::stoi(vals[fips_idx]) == locale) {
        found = true;
        break;
      }
    } catch (const std::exception&) {
      // skip unparsable fips cell
    }
  }
  if (!found)
    e.add(fmt::format("{}: no row has fips matching config locale {}.", ctx, locale));
}

}  // namespace input_verify_detail


//
// verify existenced of required files, and then their contents
//
void input_verify(const fs::path& input_dir) {
  using namespace input_verify_detail;
  Errors err;

  //
  // helpers and details verification functions
  //

      // config.json existence is guaranteed by build_model's gate above this call.
      const fs::path config_path = input_dir / "config.json";
      const auto cfg_opt = try_load_json(config_path, "config.json", err);

      bool dovax = false, do_rings = false, do_social_distancing = false;
      int locale = 0;
      if (cfg_opt)
        check_config(*cfg_opt, err, dovax, do_rings, do_social_distancing, locale);

      // Resolve a config filename key to a path under input_dir; nullopt if the key
      // is absent or not a string (already flagged by check_config).
      auto resolve = [&](const char* key) -> std::optional<fs::path> {
        if (!cfg_opt || !cfg_opt->is_object() || !cfg_opt->contains(key) ||
            !(*cfg_opt)[key].is_string())
          return std::nullopt;
        const fs::path rel = (*cfg_opt)[key].get<std::string>();
        return rel.is_absolute() ? rel : input_dir / rel;
      };

      auto verify_json_file = [&](const char* key, std::string_view label,
                                  void (*checker)(const json&, Errors&)) {
        const auto p = resolve(key);
        if (!p) return;
        if (!fs::exists(*p)) {
          err.add(fmt::format("{}: file '{}' does not exist.", label, p->string()));
          return;
        }
        const auto j = try_load_json(*p, label, err);
        if (j) checker(*j, err);
      };

  //
  // examine individual files for existence and contents
  //

      verify_json_file("seed", "seed.json", check_seed);
      verify_json_file("variants", "variants.json", check_variants);
      verify_json_file("social_params", "socialparams.json", check_socialparams);

      if (const auto geo = resolve("geodata")) {
        if (!fs::exists(*geo))
          err.add(fmt::format("geodata.csv: file '{}' does not exist.", geo->string()));
        else
          check_geodata_csv(*geo, locale, err);
      }

      if (dovax) {
        verify_json_file("vaccines", "vaccines.json", check_vaccines);
        if (const auto dir = resolve("vax_sched_dir")) {
          if (!fs::exists(*dir))
            err.add(fmt::format("vax_sched_dir: '{}' does not exist.", dir->string()));
          else if (!fs::is_directory(*dir))
            err.add(fmt::format("vax_sched_dir: '{}' is not a directory.", dir->string()));
          else {
            int json_count = 0;
            for (const auto& entry : fs::directory_iterator(*dir)) {
              if (entry.is_regular_file() && entry.path().extension() == ".json") {
                ++json_count;
                const auto j = try_load_json(entry.path(), entry.path().filename().string(), err);
                if (j) check_vax_sched(*j, entry.path().stem().string(), err);
              }
            }
            if (json_count == 0)
              err.add(fmt::format("vax_sched_dir: '{}' contains no .json schedule files.",
                                  dir->string()));
          }
        }
      }

      if (do_social_distancing)
        verify_json_file("social_dist", "soc_dist.json", check_soc_dist);

      if (do_rings)
        verify_json_file("rings", "rings.json", check_rings);

  //
  // output results if failures found
  //
      if (err.any()) {
        std::string report = fmt::format(
            "Input validation failed for '{}'. {} problem(s) found:\n",
            input_dir.string(), err.msgs.size());
        for (size_t i = 0; i < err.msgs.size(); ++i)
          report += fmt::format("  {}. {}\n", i + 1, err.msgs[i]);
        fmt::print(stderr, "{}", report);

        const fs::path log_path = fs::current_path() / "input-error-log.txt";
        if (write_error_log(log_path, report)) {
          fmt::print(stderr, "Wrote error log to {}\n", log_path.string());
        } else {
          fmt::print(stderr, "Could not write error log to {}\n", log_path.string());
        }
        std::exit(EXIT_FAILURE);
      }

      fmt::println("Input structure validated for files, json keys, and csv columns. "
                  "Not all values can be validated.  Running simulation...");
}
