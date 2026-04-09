#include "population.h"

namespace {

std::string bool_text(uint8_t value) {
  return value == 0 ? std::string{"false"} : std::string{"true"};
}

std::string format_variant_name(Variant value) {
  const auto rendered = value.show();
  if (!rendered.empty()) return rendered;
  return idx(value) == 0 ? std::string{} :
                           fmt::format("{}", static_cast<unsigned int>(static_cast<uint8_t>(value)));
}

std::string format_vax_name(Vax value) {
  const auto rendered = value.show();
  if (!rendered.empty()) return rendered;
  return idx(value) == 0 ? std::string{} :
                           fmt::format("{}", static_cast<unsigned int>(static_cast<uint8_t>(value)));
}

std::string csv_escape(std::string_view cell) {
  if (cell.find_first_of(",\"\n\r") == std::string_view::npos) {
    return std::string{cell};
  }

  std::string escaped;
  escaped.reserve(cell.size() + 2);
  escaped.push_back('"');
  for (const char ch : cell) {
    if (ch == '"') escaped.push_back('"');
    escaped.push_back(ch);
  }
  escaped.push_back('"');
  return escaped;
}

void ensure_parent_dir(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
}

std::string txt_status(AgentView person) { return person.status().show(); }

std::string txt_agegrp(AgentView person) { return person.agegrp().show(); }

std::string txt_cond(AgentView person) { return person.cond().show(); }

std::string txt_duration(AgentView person) {
  return person.duration().show();
}

std::string txt_variant(AgentView person) {
  return person.variant().show();
}

std::string txt_variant_hist(AgentView person) {
  return person.variant_hist().show();
}

std::string txt_sickday(AgentView person) {
  return person.sickday() == 0 ? std::string{} : person.sickday().show();
}

std::string txt_sickday_hist(AgentView person) {
  return person.sickday_hist().show();
}

std::string txt_recovday(AgentView person) {
  return person.recovday().show();
}

std::string txt_recovday_hist(AgentView person) {
  return person.recovday_hist().show();
}

std::string txt_deadday(AgentView person) {
  return person.deadday().show();
}

std::string txt_ring(AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.ring()));
}

std::string txt_sdcase(AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.sdcase()));
}

std::string txt_testday_hist(AgentView person) {
  return person.testday_hist().show();
}

std::string txt_testday(AgentView person) {
  return person.testday() == 0 ? std::string{} : person.testday().show();
}

std::string txt_quar(AgentView person) {
  return bool_text(person.quar());
}

std::string txt_quarday(AgentView person) {
  return person.quarday().show();
}

std::string txt_vaxstatus(AgentView person) { return person.vaxstatus().show(); }

std::string txt_vax(AgentView person) {
  return format_vax_name(person.vax());
}

std::string txt_vax_hist(AgentView person) {
  return person.vax_hist().show();
}

std::string txt_vaxday(AgentView person) {
  return person.vaxday().show();
}

std::string txt_vaxday_hist(AgentView person) {
  return person.vaxday_hist().show();
}

const PopData::PopColumnMap COLUMN_SPECS{
    {"status", {ColumnName::status, "status", txt_status}},
    {"agegrp", {ColumnName::agegrp, "agegrp", txt_agegrp}},
    {"cond", {ColumnName::cond, "cond", txt_cond}},
    {"duration", {ColumnName::duration, "duration", txt_duration}},
    {"variant", {ColumnName::variant, "variant", txt_variant}},
    {"variant_hist", {ColumnName::variant_hist, "variant_hist", txt_variant_hist}},
    {"sickday", {ColumnName::sickday, "sickday", txt_sickday}},
    {"sickday_hist", {ColumnName::sickday_hist, "sickday_hist", txt_sickday_hist}},
    {"recovday", {ColumnName::recovday, "recovday", txt_recovday}},
    {"recovday_hist", {ColumnName::recovday_hist, "recovday_hist", txt_recovday_hist}},
    {"deadday", {ColumnName::deadday, "deadday", txt_deadday}},
    {"ring", {ColumnName::ring, "ring", txt_ring}},
    {"sdcase", {ColumnName::sdcase, "sdcase", txt_sdcase}},
    {"testday_hist", {ColumnName::testday_hist, "testday_hist", txt_testday_hist}},
    {"testday", {ColumnName::testday, "testday", txt_testday}},
    {"quar", {ColumnName::quar, "quar", txt_quar}},
    {"quarday", {ColumnName::quarday, "quarday", txt_quarday}},
    {"vaxstatus", {ColumnName::vaxstatus, "vaxstatus", txt_vaxstatus}},
    {"vax", {ColumnName::vax, "vax", txt_vax}},
    {"vax_hist", {ColumnName::vax_hist, "vax_hist", txt_vax_hist}},
    {"vaxday", {ColumnName::vaxday, "vaxday", txt_vaxday}},
    {"vaxday_hist", {ColumnName::vaxday_hist, "vaxday_hist", txt_vaxday_hist}},
};

static_assert(size_t(ColumnName::COUNT) == 22);
static_assert(size_t(ColumnName::COUNT) == column_name_labels.size());

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

std::vector<PopData::PopColumnRenderer> PopData::resolve_columns(std::span<const std::string_view> col_names) {
  std::vector<PopColumnRenderer> cols;
  cols.reserve(col_names.size());
  for (const auto name : col_names) {
    const PopColumnSpec* spec = find_column(name);
    if (spec == nullptr) {
      throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", name));
    }
    cols.push_back(spec->to_txt_cell);
  }
  return cols;
}

std::vector<PopData::PopColumnRenderer> PopData::resolve_columns(const std::vector<std::string>& col_names) {
  std::vector<PopColumnRenderer> cols;
  cols.reserve(col_names.size());
  for (const auto& name : col_names) {
    const PopColumnSpec* spec = find_column(name);
    if (spec == nullptr) {
      throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", name));
    }
    cols.push_back(spec->to_txt_cell);
  }
  return cols;
}

std::vector<PopData::PopColumnRenderer> PopData::resolve_columns(std::initializer_list<std::string_view> col_names) {
  return resolve_columns(std::span<const std::string_view>{col_names.begin(), col_names.size()});
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

  const auto cols = PopData::resolve_columns(selections);

  std::filesystem::path fpath;
  if (path_steps.empty()) {
    fpath = std::filesystem::current_path() / "population_output";
  } else {
    for (const auto& step : path_steps) fpath /= step;
  }
  fpath /= make_timestamped_filename(base_fname) + ".csv";
  ensure_parent_dir(fpath);

  std::ofstream out(fpath);
  if (!out) {
    throw std::runtime_error(fmt::format("Could not write population CSV to '{}'",
                                         fpath.string()));
  }

  std::vector<std::string> header;
  header.reserve(selections.size() + 1);
  header.push_back("row");
  for (const auto& col : selections) header.push_back(col);
  fmt::println(out, "{}", fmt::join(header, ","));

  for (size_t person_idx = 1; person_idx <= popn; ++person_idx) {
    std::vector<std::string> row;
    row.reserve(cols.size() + 1);
    row.push_back(fmt::format("{}", person_idx));
    const auto person = agent(person_idx);
    for (const auto& col : cols) {
      row.push_back(col(person));
    }
    fmt::println(out, "{}", fmt::join(row, ","));
  }

  fmt::println("Wrote selected population CSV to '{}'", fpath.string());
}
