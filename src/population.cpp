#include "population.h"

namespace {

std::string format_variant_name(Variant value) {
  const auto idx_value = idx(value);
  if (!Variant::names.empty() && idx_value < Variant::names.size()) {
    return Variant::names[idx_value];
  }
  return fmt::format("{}", static_cast<unsigned int>(static_cast<uint8_t>(value)));
}

template <typename T, size_t N, typename Formatter>
std::string join_history_values(const std::array<T, N>& values, uint8_t count,
                                Formatter formatter) {
  const size_t entry_count = std::min<size_t>(count, N);
  if (entry_count == 0) return "";

  std::vector<std::string> rendered;
  rendered.reserve(entry_count);
  for (size_t entry_idx = 0; entry_idx < entry_count; ++entry_idx) {
    rendered.push_back(formatter(values[entry_idx]));
  }
  return fmt::format("{}", fmt::join(rendered, "|"));
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

std::string txt_status(PopData::AgentView person) { return person.status().name(); }

std::string txt_agegrp(PopData::AgentView person) { return person.agegrp().name(); }

std::string txt_cond(PopData::AgentView person) { return person.cond().name(); }

std::string txt_duration(PopData::AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.duration()));
}

std::string txt_variant(PopData::AgentView person) {
  return join_history_values(person.all_variants(), person.variant_count(),
                             [](Variant value) { return format_variant_name(value); });
}

std::string txt_variant_count(PopData::AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.variant_count()));
}

std::string txt_sickday(PopData::AgentView person) {
  return join_history_values(person.all_sickdays(), person.variant_count(),
                             [](int16_t value) { return fmt::format("{}", value); });
}

std::string txt_recovday(PopData::AgentView person) {
  return join_history_values(person.all_recovdays(), person.recovday_count(),
                             [](int16_t value) { return fmt::format("{}", value); });
}

std::string txt_recovday_count(PopData::AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.recovday_count()));
}

std::string txt_deadday(PopData::AgentView person) {
  return fmt::format("{}", person.deadday());
}

std::string txt_ring(PopData::AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.ring()));
}

std::string txt_sdcase(PopData::AgentView person) {
  return person.bool_labels().to_str(person.sdcase());
}

std::string txt_tested(PopData::AgentView person) {
  return join_history_values(person.tested(), person.tested_count(),
                             [&](uint8_t value) { return person.bool_labels().to_str(value); });
}

std::string txt_tested_count(PopData::AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.tested_count()));
}

std::string txt_testday(PopData::AgentView person) {
  return join_history_values(person.testday(), person.tested_count(),
                             [](int16_t value) { return fmt::format("{}", value); });
}

std::string txt_quar(PopData::AgentView person) {
  return person.bool_labels().to_str(person.quar());
}

std::string txt_quarday(PopData::AgentView person) {
  return fmt::format("{}", person.quarday());
}

std::string txt_vaxstatus(PopData::AgentView person) { return person.vaxstatus().name(); }

std::string txt_vaxrcvd(PopData::AgentView person) {
  return join_history_values(person.vaxrcvd(), person.vax_count(),
                             [&](uint8_t value) { return person.vax_labels().to_str(value); });
}

std::string txt_vax_count(PopData::AgentView person) {
  return fmt::format("{}", static_cast<unsigned int>(person.vax_count()));
}

std::string txt_vaxday(PopData::AgentView person) {
  return join_history_values(person.vaxday(), person.vax_count(),
                             [](int16_t value) { return fmt::format("{}", value); });
}

const PopData::PopColumnMap COLUMN_SPECS{
    {"status", {ColumnName::status, "status", txt_status}},
    {"agegrp", {ColumnName::agegrp, "agegrp", txt_agegrp}},
    {"cond", {ColumnName::cond, "cond", txt_cond}},
    {"duration", {ColumnName::duration, "duration", txt_duration}},
    {"variant", {ColumnName::variant, "variant", txt_variant}},
    {"variant_count", {ColumnName::variant_count, "variant_count", txt_variant_count}},
    {"sickday", {ColumnName::sickday, "sickday", txt_sickday}},
    {"recovday", {ColumnName::recovday, "recovday", txt_recovday}},
    {"recovday_count", {ColumnName::recovday_count, "recovday_count", txt_recovday_count}},
    {"deadday", {ColumnName::deadday, "deadday", txt_deadday}},
    {"ring", {ColumnName::ring, "ring", txt_ring}},
    {"sdcase", {ColumnName::sdcase, "sdcase", txt_sdcase}},
    {"tested", {ColumnName::tested, "tested", txt_tested}},
    {"tested_count", {ColumnName::tested_count, "tested_count", txt_tested_count}},
    {"testday", {ColumnName::testday, "testday", txt_testday}},
    {"quar", {ColumnName::quar, "quar", txt_quar}},
    {"quarday", {ColumnName::quarday, "quarday", txt_quarday}},
    {"vaxstatus", {ColumnName::vaxstatus, "vaxstatus", txt_vaxstatus}},
    {"vaxrcvd", {ColumnName::vaxrcvd, "vaxrcvd", txt_vaxrcvd}},
    {"vax_count", {ColumnName::vax_count, "vax_count", txt_vax_count}},
    {"vaxday", {ColumnName::vaxday, "vaxday", txt_vaxday}},
};

static_assert(size_t(ColumnName::COUNT) == 21);
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

std::vector<const PopData::PopColumnSpec*> PopData::resolve_columns(std::span<const std::string_view> col_names) {
  std::vector<const PopColumnSpec*> cols;
  cols.reserve(col_names.size());
  for (const auto name : col_names) {
    const PopColumnSpec* spec = find_column(name);
    if (spec == nullptr) {
      throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", name));
    }
    cols.push_back(spec);
  }
  return cols;
}

std::vector<const PopData::PopColumnSpec*> PopData::resolve_columns(const std::vector<std::string>& col_names) {
  std::vector<const PopColumnSpec*> cols;
  cols.reserve(col_names.size());
  for (const auto& name : col_names) {
    const PopColumnSpec* spec = find_column(name);
    if (spec == nullptr) {
      throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", name));
    }
    cols.push_back(spec);
  }
  return cols;
}

std::vector<const PopData::PopColumnSpec*> PopData::resolve_columns(std::initializer_list<std::string_view> col_names) {
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
  header.reserve(cols.size() + 1);
  header.push_back("row");
  for (const auto* col : cols) header.push_back(std::string{col->key});
  fmt::println(out, "{}", fmt::join(header, ","));

  for (size_t person_idx = 1; person_idx <= popn; ++person_idx) {
    std::vector<std::string> row;
    row.reserve(cols.size() + 1);
    row.push_back(fmt::format("{}", person_idx));
    const auto person = agent(person_idx);
    for (const auto* col : cols) {
      row.push_back(csv_escape(col->to_txt_cell(person)));
    }
    fmt::println(out, "{}", fmt::join(row, ","));
  }

  fmt::println("Wrote selected population CSV to '{}'", fpath.string());
}
