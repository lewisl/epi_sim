#include "population.h"
#include "pop_serialize.h"

namespace {

void ensure_parent_dir(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
}

std::string txt_status(AgentView person) { return render_agent_pop_cell("status", person); }
std::string txt_agegrp(AgentView person) { return render_agent_pop_cell("agegrp", person); }
std::string txt_cond(AgentView person) { return render_agent_pop_cell("cond", person); }
std::string txt_duration(AgentView person) { return render_agent_pop_cell("duration", person); }
std::string txt_variant(AgentView person) { return render_agent_pop_cell("variant", person); }
std::string txt_variant_hist(AgentView person) { return render_agent_pop_cell("variant_hist", person); }
std::string txt_sickday(AgentView person) { return render_agent_pop_cell("sickday", person); }
std::string txt_sickday_hist(AgentView person) { return render_agent_pop_cell("sickday_hist", person); }
std::string txt_recovday(AgentView person) { return render_agent_pop_cell("recovday", person); }
std::string txt_recovday_hist(AgentView person) { return render_agent_pop_cell("recovday_hist", person); }
std::string txt_deadday(AgentView person) { return render_agent_pop_cell("deadday", person); }
std::string txt_ring(AgentView person) { return render_agent_pop_cell("ring", person); }
std::string txt_sdcase(AgentView person) { return render_agent_pop_cell("sdcase", person); }
std::string txt_testday_hist(AgentView person) { return render_agent_pop_cell("testday_hist", person); }
std::string txt_testday(AgentView person) { return render_agent_pop_cell("testday", person); }
std::string txt_quar(AgentView person) { return render_agent_pop_cell("quar", person); }
std::string txt_quarday(AgentView person) { return render_agent_pop_cell("quarday", person); }
std::string txt_vaxstatus(AgentView person) { return render_agent_pop_cell("vaxstatus", person); }
std::string txt_vax(AgentView person) { return render_agent_pop_cell("vax", person); }
std::string txt_vax_hist(AgentView person) { return render_agent_pop_cell("vax_hist", person); }
std::string txt_vaxday(AgentView person) { return render_agent_pop_cell("vaxday", person); }
std::string txt_vaxday_hist(AgentView person) { return render_agent_pop_cell("vaxday_hist", person); }

// Registry keys must match ColumnName / column_name_labels; static_assert below guards COUNT.
const PopData::PopColumnMap COLUMN_SPECS{
    {"status", {ColumnName::status, txt_status}},
    {"agegrp", {ColumnName::agegrp, txt_agegrp}},
    {"cond", {ColumnName::cond, txt_cond}},
    {"duration", {ColumnName::duration, txt_duration}},
    {"variant", {ColumnName::variant, txt_variant}},
    {"variant_hist", {ColumnName::variant_hist, txt_variant_hist}},
    {"sickday", {ColumnName::sickday, txt_sickday}},
    {"sickday_hist", {ColumnName::sickday_hist, txt_sickday_hist}},
    {"recovday", {ColumnName::recovday, txt_recovday}},
    {"recovday_hist", {ColumnName::recovday_hist, txt_recovday_hist}},
    {"deadday", {ColumnName::deadday, txt_deadday}},
    {"ring", {ColumnName::ring, txt_ring}},
    {"sdcase", {ColumnName::sdcase, txt_sdcase}},
    {"testday_hist", {ColumnName::testday_hist, txt_testday_hist}},
    {"testday", {ColumnName::testday, txt_testday}},
    {"quar", {ColumnName::quar, txt_quar}},
    {"quarday", {ColumnName::quarday, txt_quarday}},
    {"vaxstatus", {ColumnName::vaxstatus, txt_vaxstatus}},
    {"vax", {ColumnName::vax, txt_vax}},
    {"vax_hist", {ColumnName::vax_hist, txt_vax_hist}},
    {"vaxday", {ColumnName::vaxday, txt_vaxday}},
    {"vaxday_hist", {ColumnName::vaxday_hist, txt_vaxday_hist}},
};

static_assert(size_t(ColumnName::COUNT) == 22);
static_assert(size_t(ColumnName::COUNT) == column_name_labels.size());

void print_valid_popdata_column_names_hint() {
  std::vector<std::string_view> keys;
  keys.reserve(COLUMN_SPECS.size());
  for (const auto& [k, spec] : COLUMN_SPECS) {
    (void)spec;
    keys.push_back(k);
  }
  std::ranges::sort(keys);
  fmt::println("Valid PopData columns: {}", fmt::join(keys, ", "));
}

}  // namespace

std::optional<ColumnName> PopData::column_name_from_string(std::string_view text) {
  const PopColumnSpec* spec = find_column(text);
  return spec ? std::optional<ColumnName>{spec->name} : std::nullopt;
}

const PopData::PopColumnMap& PopData::column_map() { return COLUMN_SPECS; }

const PopData::PopColumnSpec* PopData::find_column(ColumnName name) {
  if (size_t(name) >= column_name_labels.size()) return nullptr;
  return find_column(to_string(name));
}

const PopData::PopColumnSpec* PopData::find_column(std::string_view key) {
  const auto it = COLUMN_SPECS.find(key);
  if (it == COLUMN_SPECS.end()) return nullptr;
  return &it->second;
}


void PopData::serialize_selected_columns(std::vector<string> selections,
                                         string base_fname, vector<string> path_steps) {
  if (popn == 0) {
    fmt::println("\nNo population data to output.");
    return;
  }

  if (selections.empty()) {
    fmt::println("\nNo columns selected");
    return;
  }

  // Unknown names are skipped (with a hint); duplicates resolve to the first occurrence.
  std::vector<std::string_view> selected_columns;
  std::vector<std::string> invalid;
  std::array<bool, static_cast<size_t>(ColumnName::COUNT)> used{};
  selected_columns.reserve(selections.size());

  for (const auto& col_lbl : selections) {
    const PopColumnSpec* spec = find_column(std::string_view{col_lbl});
    if (spec == nullptr) {
      invalid.push_back(col_lbl);
      continue;
    }
    const auto ord = static_cast<size_t>(spec->name);
    if (ord >= used.size()) {
      continue;
    }
    if (used[ord]) {
      continue;
    }
    used[ord] = true;
    selected_columns.push_back(col_lbl);
  }

  auto print_column_hints = [&] {      // TODO Does this have to be a lambda?  is it to do the closure? only enclosing var invalid
    if (!invalid.empty()) {
      fmt::println("Skipping unknown PopData column(s): {}", fmt::join(invalid, ", "));
    }
    print_valid_popdata_column_names_hint();
  };

  if (selected_columns.empty()) {
    fmt::println("\nNo valid columns selected for population CSV output.");
    print_column_hints();
    return;
  }

  if (!invalid.empty()) {
    print_column_hints();
  }

  // Match serialize_selected_series: non-empty path_steps are under $HOME (each step appended).
  // If a step is an absolute path, path /= replaces to that path (tests use temp dirs this way).
  std::filesystem::path fpath;
  if (path_steps.empty()) {
    fpath = std::filesystem::current_path() / "population_output";
  } else {
    const char* home = std::getenv("HOME");
    if (!home) throw std::runtime_error("HOME not set");
    fpath = home;
    for (const auto& step : path_steps) fpath /= step;
  }
  fpath /= make_timestamped_filename(base_fname) + ".csv";
  ensure_parent_dir(fpath);

  std::ofstream out(fpath);  // only create the file handle after verifying we have things to write
  if (!out) {
    throw std::runtime_error(fmt::format("Could not write population CSV to '{}'",
                                         fpath.string()));
  }

  std::vector<size_t> rows;
  rows.reserve(popn);
  for (size_t person_idx = 1; person_idx <= popn; ++person_idx) {
    rows.push_back(person_idx);
  }

  write_agent_pop_data(*this, rows, selected_columns, out, PopOutputLayout::serialized, false,
                       ",", true);

  fmt::println("\nWrote selected population CSV to '{}'", fpath.string());
}
