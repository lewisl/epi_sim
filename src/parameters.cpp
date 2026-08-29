#include "parameters.h"
#include "helpers.h"    // for shifter range compressor
#include <charconv>
#include <cstdint>
#include <csv2/reader.hpp>


// using json = nlohmann::json;
using json = nlohmann::ordered_json;
// using std::array;
using std::string;
using std::vector;



json load_json_params(string fpath) {

  try {
    std::ifstream fcontent(fpath);
    json data = json::parse(fcontent, nullptr, true, true);  // json data = json::parse(fcontent, nullptr, true, true);

    return data;
  }

  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
      throw std::invalid_argument(fmt::format("Invalid file path for json file: {}", fpath)); 
  }
}


//
// geodata
//
GeoData load_geodata_csv(const std::string& filename) {
  using namespace csv2;

  GeoData data;
    
  Reader<delimiter<','>,       // this is a templated class instance called csv
    quote_character<'"'>, 
    first_row_is_header<true>,
    trim_policy::trim_whitespace> csv;
  
  if (!csv.mmap(filename)) {
      throw std::runtime_error("Failed to open CSV file: " + filename);
  }

  // Read all rows
  std::vector<std::string> values; // hold cells per row
  values.reserve(20);  // saves allocations for most elements per row cases
  for (const auto& row : csv) {
    values.clear();   
    for (const auto& cell : row) {
        std::string value;
        cell.read_value(value);
        values.push_back(value);
    }
      
    // push into typed columns (order matches CSV)
    data.fips.push_back(std::stoi(values[0]));
    data.county.push_back(values[1]);
    data.city.push_back(values[2]);
    data.state.push_back(values[3]);
    data.sizecat.push_back(std::stoi(values[4]));
    data.pop.push_back(std::stoi(values[5]));
    data.density.push_back(std::stof(values[6]));
    data.anchor.push_back(values[7]);
    data.indoor_st.push_back(values[8]);
    data.indoor_end.push_back(values[9]);
      
    ++data.num_rows;
  }

  shifter(data.density, 0.9, 1.25);  // turn population density of cities into a compressed index
  return data;
}


//
// variant data supplies infectparams, progressionset, and variant names
//


std::tuple<vector<string>, vector<InfectParams>> load_variants_data(json jdata) {

  Variant::names.clear();
  Variant::names = {"none"};
  vector<string> variant_names{"none"};

  for (auto v : jdata.items()) {
    variant_names.emplace_back(v.key());
    const Variant variant(v.key());
  }

  if (variant_names.size() < 2) {
    throw std::runtime_error("No variants loaded from json file of variants. Can't run simulation.");
  }

  const Variant &primary = Variant{1};
  if (primary.show() != "base") {
    throw std::runtime_error(fmt::format(
        "variants: the first variant in the JSON file must be named 'base' (got '{}'). "
        "Downstream code (spread, progression, r0/rt estimation) assumes variants[1] is the reference variant.",
        primary.show()));
  }

  vector<InfectParams> infectparams{};
  // Add a dummy "none" entry at index 0 to align with variants
  infectparams.emplace_back(InfectParams{});

  for (auto variant : jdata.items()) {

    auto recovery_imm_obj = variant.value()["immunity"]["recovery_immunity"];
    
    // Build the recovery_immunity vector in the correct order:  
    //      invariant: variant_names must be in same order as infectparams
    vector<float> recovery_immunity(Variant::names.size(), 0.0f);  // default to 0
    for (size_t i = 0; i < Variant::names.size(); ++i) {
        const auto& vname = Variant::names[i];
        if (recovery_imm_obj.contains(vname)) {
            recovery_immunity[i] = recovery_imm_obj[vname].get<float>();
        }
    }

    const auto raw_sendrisk = variant.value()["spread"]["sendrisk"].get<vector<float>>();
    vector<float> sendrisk;
    float base = variant.value()["spread"]["basemultiplier"].get<float>();
    if (raw_sendrisk.size() == 0) {
      if (infectparams.size() < 2) {
        throw std::runtime_error(fmt::format(
            "variants: '{}' (the base variant) must supply non-empty sendrisk values; "
            "there is no earlier variant to derive them from.", variant.key()));
      }
      sendrisk = infectparams[1].sendrisk;
      std::transform(sendrisk.begin(), sendrisk.end(), sendrisk.begin(), [base](float x) {return x * base;} );
    } else {
      sendrisk = raw_sendrisk;
    }

    const auto raw_recvrisk = variant.value()["spread"]["recvrisk"].get<vector<float>>();
    vector<float> recvrisk;
    // use basemultiplier from above...
    if (raw_recvrisk.size() == 0) {
      if (infectparams.size() < 2) {
        throw std::runtime_error(fmt::format(
            "variants: '{}' (the base variant) must supply non-empty recvrisk values; "
            "there is no earlier variant to derive them from.", variant.key()));
      }
      recvrisk = infectparams[1].recvrisk;
      std::transform(recvrisk.begin(), recvrisk.end(), recvrisk.begin(), [base](float x) {return x * base;} );
    } else {
      recvrisk = raw_recvrisk;
    }

    infectparams.emplace_back(InfectParams{
        .sendrisk = std::move(sendrisk),
        .recvrisk = std::move(recvrisk),
        .recovery_immunity = std::move(recovery_immunity),
        .basemultiplier = variant.value()["spread"]["basemultiplier"].get<float>(),
        .immunehalflife = variant.value()["immunity"]["immunehalflife"]});
  }
  return {variant_names, infectparams};
}

/*
The JSON progression tree is converted once into a packed lookup table:
  [age][duration] -> packed breakday entry -> [current condition][outcome]
The six outcomes are recover, nil, mild, sick, severe, and dead.
*/

namespace {

uint8_t parse_progression_day(std::string_view text, std::string_view variant,
                              std::string_view age) {
  unsigned day{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), day);
  if (error != std::errc{} || end != text.data() + text.size() ||
      day < 1 || day > DURATIONLIM) {
    throw std::runtime_error(fmt::format(
        "progression_tree: variant '{}' age '{}' has invalid breakday '{}'; "
        "expected an integer in 1..{}.",
        variant, age, text, DURATIONLIM));
  }
  return static_cast<uint8_t>(day);
}

void validate_terminal_progression(const ProgressionTree& tree,
                                   std::string_view variant) {
  for (size_t age_idx = 0; age_idx < PROGRESSION_AGE_COUNT; ++age_idx) {
    const int16_t terminal_entry = tree.entry_index[age_idx][DURATIONLIM];
    if (terminal_entry == NO_PROGRESSION_ENTRY) {
      throw std::runtime_error(fmt::format(
          "progression_tree: variant '{}' age '{}' must define terminal breakday {}.",
          variant, Agegrp::names[age_idx + 1], DURATIONLIM));
    }

    const auto& condition_rows = tree.entries[static_cast<size_t>(terminal_entry)];
    for (size_t cond_idx = 0; cond_idx < condition_rows.size(); ++cond_idx) {
      for (size_t outcome = Progressmap::ToNil; outcome <= Progressmap::ToSevere;
           ++outcome) {
        if (!approx_equal(condition_rows[cond_idx][outcome], 0.0, 1e-6)) {
          throw std::runtime_error(fmt::format(
              "progression_tree: variant '{}' age '{}' day {} condition '{}' "
              "must transition only to recover or dead.",
              variant, Agegrp::names[age_idx + 1], DURATIONLIM,
              Condition::names[cond_idx + 1]));
        }
      }
    }
  }
}

}  // namespace

ProgressionSet load_progression_set(json jdata) {
  ProgressionSet progressionset{};

  // Index 0 aligns with Variant::none and is never used by progression().
  progressionset.progression.emplace_back();

  for (const auto &[variant, body] : jdata.items()) {
    ProgressionFactors factors{};
    const auto& jsontree = body["progression_tree"];
    const auto& jsonfactors = body["progression_factors"];

    factors.riskadjust = jsonfactors["riskadjust"].get<vector<float>>();
    if (!factors.riskadjust.empty() &&
        factors.riskadjust.size() != PROGRESSION_OUTCOME_COUNT) {
      throw std::runtime_error(fmt::format(
          "progression_factors: variant '{}' riskadjust must be empty or contain "
          "exactly {} values, got {}.",
          variant, PROGRESSION_OUTCOME_COUNT, factors.riskadjust.size()));
    }
    for (const auto &[vax, num] :
         jsonfactors["vaxhalflifeadjust"].get<absl::flat_hash_map<string, float>>()) {
      factors.vaxhalflifeadjust[vax] = num;
    }

    ProgressionTree tree;
    if (jsontree.is_null()) {
      if (progressionset.progression.size() <= 1 ||
          progressionset.progression[1].tree.entries.empty()) {
        throw std::runtime_error(fmt::format(
            "progression_tree: variant '{}' cannot inherit before a non-null base tree is loaded.",
            variant));
      }

      tree = progressionset.progression[1].tree;
      if (!factors.riskadjust.empty()) {
        for (auto& condition_rows : tree.entries) {
          for (auto& row : condition_rows) {
            for (size_t i = 0; i < row.size(); ++i) row[i] *= factors.riskadjust[i];
            const float sum = std::accumulate(row.begin(), row.end(), 0.0f);
            if (sum <= 0.0f) {
              throw std::runtime_error(fmt::format(
                  "progression_factors: variant '{}' riskadjust produces a zero-sum row.",
                  variant));
            }
            for (float& value : row) value /= sum;
          }
        }
      }
    } else {
      array<bool, PROGRESSION_AGE_COUNT> seen_ages{};
      size_t breakday_count = 0;
      for (const auto& [age, body_age] : jsontree.items()) {
        breakday_count += body_age.size();
      }
      tree.entries.reserve(breakday_count);

      for (const auto& [age, body_age] : jsontree.items()) {
        const auto age_it = std::find(Agegrp::names.begin() + 1,
                                      Agegrp::names.end(), age);
        if (age_it == Agegrp::names.end()) {
          throw std::runtime_error(fmt::format(
              "progression_tree: variant '{}' has unknown age group '{}'.",
              variant, age));
        }
        const size_t age_idx =
            static_cast<size_t>(std::distance(Agegrp::names.begin(), age_it) - 1);
        if (seen_ages[age_idx]) {
          throw std::runtime_error(fmt::format(
              "progression_tree: variant '{}' repeats age group '{}'.", variant, age));
        }
        seen_ages[age_idx] = true;

        for (const auto& [duration, body_duration] : body_age.items()) {
          const uint8_t day = parse_progression_day(duration, variant, age);
          if (tree.entry_index[age_idx][day] != NO_PROGRESSION_ENTRY) {
            throw std::runtime_error(fmt::format(
                "progression_tree: variant '{}' age '{}' repeats breakday {}.",
                variant, age, day));
          }

          OutcomesByCurrentCondition condition_rows{};
          for (size_t cond_idx = 0; cond_idx < PROGRESSION_CONDITION_COUNT;
               ++cond_idx) {
            const string& key = Condition::names[cond_idx + 1];
            const auto& json_row = body_duration[key];
            if (!json_row.is_array() || json_row.size() != PROGRESSION_OUTCOME_COUNT) {
              throw std::runtime_error(fmt::format(
                  "progression_tree: variant '{}' age '{}' day '{}' condition '{}' "
                  "must have exactly {} probabilities (recover,nil,mild,sick,severe,dead), got {}.",
                  variant, age, duration, key, PROGRESSION_OUTCOME_COUNT,
                  json_row.is_array() ? json_row.size() : 0));
            }

            auto& row = condition_rows[cond_idx];
            for (size_t outcome = 0; outcome < row.size(); ++outcome) {
              row[outcome] = json_row[outcome].get<float>();
              if (row[outcome] < 0.0f || row[outcome] > 1.0f) {
                throw std::runtime_error(fmt::format(
                    "progression_tree: variant '{}' age '{}' day '{}' condition '{}' "
                    "probability {} must be in [0,1] (got {}).",
                    variant, age, duration, key, outcome, row[outcome]));
              }
            }
            const float row_sum = std::accumulate(row.begin(), row.end(), 0.0f);
            if (!approx_equal(row_sum, 1.0, 1e-6)) {
              throw std::runtime_error(fmt::format(
                  "progression_tree: variant '{}' age '{}' day '{}' condition '{}' "
                  "probabilities must sum to 1.0 (got {}).",
                  variant, age, duration, key, row_sum));
            }
          }

          const size_t entry_idx = tree.entries.size();
          tree.entries.push_back(std::move(condition_rows));
          tree.entry_index[age_idx][day] = static_cast<int16_t>(entry_idx);
        }
      }

      for (size_t age_idx = 0; age_idx < seen_ages.size(); ++age_idx) {
        if (!seen_ages[age_idx]) {
          throw std::runtime_error(fmt::format(
              "progression_tree: variant '{}' is missing age group '{}'.",
              variant, Agegrp::names[age_idx + 1]));
        }
      }
      validate_terminal_progression(tree, variant);
    }

    progressionset.progression.push_back(Progression{
        .tree = std::move(tree),
        .factors = std::move(factors),
    });
  }

  return progressionset;
}

std::tuple<vector<InfectParams>, ProgressionSet, vector<string>>
load_infect_params(string fpath) {
  json jdata = load_json_params(fpath);
  auto [variant_names, infectparams] = load_variants_data(jdata);
  ProgressionSet progressionset = load_progression_set(jdata);
  return {std::move(infectparams), std::move(progressionset),
          std::move(variant_names)};
}

//
// vaccine data
//

namespace {

std::optional<Vax> find_vax(std::string_view name) {
  const auto it = std::find(Vax::names.begin(), Vax::names.end(), name);
  if (it == Vax::names.end()) return std::nullopt;
  return Vax{static_cast<uint8_t>(std::distance(Vax::names.begin(), it))};
}

}  // namespace

VaxSet load_vax_data(string fpath) {
  VaxSet vaxset{};

  json vaxdata = load_json_params(fpath); // read data from json file input

  // 2 different but parallel use of vax names:
  Vax::names.clear();  // static in the struct with multiple instances sharing static member
  vaxset.names.clear();  // instance variable vaxset updating "local" names member
  Vax::names.emplace_back("none");
  vaxset.names.emplace_back("none");
  vaxset.params.clear();
  vaxset.params.emplace_back(Vaxparam{});

  // for each vax
  for (const auto &[vaxname, body] : vaxdata.items()) {  
    const Vax vax{vaxname};  // create a Vax trait instance
    vaxset.names.emplace_back(vaxname);  // add element to vaxset.names
    (void)vax;

    Vaxparam vx {};  // details for one vaccine
    // load items for each vax into struct
    vx.reqdshots = body["reqdshots"];
    vx.delay2ndshot = body["delay2ndshot"];
    vx.delaybooster = body["delaybooster"];
    vx.halflife = body["halflife"];
    vx.full_effect_days = body["full_effect_days"];
    vx.day1_effect = body["day1_effect"];

    // infectfactor vector
    /*
    infectfactor vector contains factors for 
    Variants[1..] with no 0th position for "none".  
    so to access it using zero based indexing,
     where 0th index accesses the first variant's factor.
     Used in disease_modeling.cpp functions.
    */
    for (const auto &variantname : Variant::names) {
      if (body["infectfactor"].contains(variantname) )
        vx.infectfactor.emplace_back(variantname, body["infectfactor"][variantname]);
      else
        if (variantname != "none")
          fmt::println("\nWARNING: variant effectiveness missing for vax: {} variant {}\n",
              vaxname, variantname);
    }

    // effectiveness vector of vector
    for (size_t shot_idx = 1; shot_idx < Vaxstatus::names.size(); ++shot_idx) {
      const auto& shot = Vaxstatus::names[shot_idx];
      vector<std::pair<string, float>> variant_effectiveness {};
      for (const auto &variantname : Variant::names) {
        if (body["effectiveness"][shot].contains(variantname))
          variant_effectiveness.emplace_back(variantname, body["effectiveness"][shot][variantname]);
        else 
          if (variantname != "none")
            fmt::println("\nWARNING: variant effectiveness missing for vax: {} shot {} variant {}\n",
                vaxname, shot, variantname);
      };
      vx.effectiveness.emplace_back(shot, variant_effectiveness);
    }
    vaxset.params.push_back(vx);
  }

  return vaxset;
};

static Agegrp agegrp_from_string(const string& s) {
    // lowercase s and compare against lowercased Agegrp names
    string sl = s;
    std::transform(sl.begin(), sl.end(), sl.begin(), ::tolower);
    for (size_t i = 0; i < Agegrp::names.size(); ++i) {
        string nl = Agegrp::names[i];
        std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);
        // also handle underscore vs no-underscore: "age80_up" vs "age80up"
        nl.erase(std::remove(nl.begin(), nl.end(), '_'), nl.end());
        sl.erase(std::remove(sl.begin(), sl.end(), '_'), sl.end());  // strip both
        if (sl == nl) return Agegrp{static_cast<uint8_t>(i)};
    }
    fmt::println("WARNING: unknown agegrp filter string: {}", s);
    return UNKNOWN;
}


VaxSched load_vax_sched(const string &fname) {
  json jdata = load_json_params(fname);
  
  VaxSched sched{};

  //  vaxesincluded member
  for (const auto &[vax, factors] : jdata["vaxesincluded"].items()) {
    PerVaxSpec spec{};
    const auto resolved_vax = find_vax(vax);
    if (!resolved_vax.has_value()) {
      fmt::println("WARNING: Vaccine {} not found in Vax::names.", vax);
      continue;
    }
    spec.vax = *resolved_vax;
    spec.mix = factors["mix"];
    spec.starting_doses = factors["starting_doses"];
    spec.pct2ndshot = factors["pct2ndshot"];
    spec.pctboost = factors["pctboost"];
    for (const auto& alt_name : factors["alternate"]) {
      const auto resolved_alt = find_vax(alt_name.get<string>());
      if (!resolved_alt.has_value()) {
        fmt::println("WARNING: Alternate vaccine {} not found in Vax::names.", alt_name.get<string>());
        continue;
      }
      spec.alternate.push_back(*resolved_alt);
    }
    sched.vaxesincluded.push_back(spec);
  };
  const float mix_sum = std::accumulate(sched.vaxesincluded.begin(), sched.vaxesincluded.end(), 0.0f,
                                        [](float acc, const PerVaxSpec& s) { return acc + s.mix; });
  if (!sched.vaxesincluded.empty() && !approx_equal(mix_sum, 1.0, 1e-6)) {
    throw std::runtime_error(fmt::format(
        "vax schedule '{}': mix values across vaxesincluded must sum to 1.0 (got {}).",
        fname, mix_sum));
  }
  // other members
  sched.dayrange = {jdata["dayrange"][0], jdata["dayrange"][1]}; // vector of 2 set to pair
  sched.targetpct = jdata["targetpct"];
  for (const auto& f : jdata["filtervec"])
    sched.filtervec.push_back(agegrp_from_string(f.get<string>()));
  sched.shotmode = jdata["shotmode"];
  sched.pattern.assign(jdata["pattern"].begin(), jdata["pattern"].end());
  sched.init_spreadfunc();
  sched.reset_doses();

  return sched;
}

VaxSchedSet load_vax_sched_set(const string &dirpath) {
  namespace fs = std::filesystem;

  VaxSchedSet schedset{};
  vector<fs::path> schedule_files;

  if (!fs::exists(dirpath)) {
    throw std::runtime_error("Vaccine schedule directory does not exist: " + dirpath);
  }
  if (!fs::is_directory(dirpath)) {
    throw std::runtime_error("Vaccine schedule path is not a directory: " + dirpath);
  }

  for (const auto& entry : fs::directory_iterator(dirpath)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      schedule_files.push_back(entry.path());
    }
  }

  std::sort(schedule_files.begin(), schedule_files.end());

  for (const auto& path : schedule_files) {
    string sched_name = path.stem().string();
    auto it = std::find_if(schedset.schedules.begin(), schedset.schedules.end(),
                           [&](const auto& entry) { return entry.first == sched_name; });
    if (it != schedset.schedules.end()) {
      throw std::runtime_error("Duplicate vaccine schedule name from filename stem: " + sched_name);
    }
    schedset.schedules.emplace_back(sched_name, load_vax_sched(path.string()));
  }

  return schedset;
}






SocialParams load_social_params(string social_path) {
  json data = load_json_params(social_path);

  SocialParams socialp;
  socialp.gammashape = data["gammashape"];
  socialp.indoor_uplift = data["indoor_uplift"];

  // build touchfactors by rows (using struct's const members)
  for (size_t rowidx = 0; rowidx < socialp.touch_rows.size(); ++rowidx) {
    for (size_t colidx = 0; colidx < socialp.age_columns.size(); ++colidx) {
      socialp.touchfactors[rowidx][colidx] = data["touchfactors"][socialp.age_columns[colidx]][socialp.touch_rows[rowidx]]; // effectively transposing the json
    }
  }

  // build contactfactors by rows (using struct's const members)
  for (size_t rowidx = 0; rowidx < socialp.contact_rows.size(); ++rowidx) {
    for (size_t colidx = 0; colidx < socialp.age_columns.size(); ++colidx) {
      socialp.contactfactors[rowidx][colidx] = data["contactfactors"][socialp.age_columns[colidx]][socialp.contact_rows[rowidx]]; // effectively transposing the json
    }
  }

  return socialp;
}

//
// ring traits
//
RingTraits load_ring_traits(string fpath) {
  RingTraits rt{};
  Ring::names.clear();
  vector<string> ring_names{""};
  Ring::names = {""};

  json data = load_json_params(fpath);
  if (!data.contains("rings")) {
    throw std::runtime_error("rings: expected top-level 'rings' key.");
  }

  const auto& ring_arr = data["rings"];
  if (!ring_arr.is_array()) {
    throw std::runtime_error("rings: expected an array under top-level 'rings' key.");
  }
  if (ring_arr.empty()) {
    throw std::runtime_error("rings: expected at least one ring.");
  }

  const size_t nrings = ring_arr.size();
  // index 0 in both vectors is the unused 1-based sentinel
  rt.pct_of_population.assign(nrings + 1, 0.0f);
  rt.out_ring_prob.assign(nrings + 1, vector<float>(Agegrp::names.size(), 0.0f));

  double pct_sum = 0.0;
  for (size_t n = 1; n <= nrings; ++n) {
    const auto& entry = ring_arr[n - 1];

    string name = entry.contains("name") && !entry["name"].is_null()
                      ? entry["name"].get<string>()
                      : fmt::format("ring_{}", n);
    if (std::find(Ring::names.begin(), Ring::names.end(), name) != Ring::names.end()) {
      throw std::runtime_error(fmt::format("rings: duplicate ring name '{}'.", name));
    }
    ring_names.emplace_back(name);

    Ring registered{std::string_view{name}};
    if (static_cast<size_t>(registered.v) != n) {
      throw std::runtime_error(fmt::format(
          "rings: registration order mismatch for '{}' (got id {}, expected {}).",
          name, static_cast<unsigned>(registered.v), n));
    }

    if (!entry.contains("pct_of_population")) {
      throw std::runtime_error(fmt::format("rings: '{}' missing pct_of_population.", name));
    }
    const float pct = entry["pct_of_population"].get<float>();
    if (pct < 0.0f || pct > 1.0f) {
      throw std::runtime_error(fmt::format(
          "rings: '{}' pct_of_population={} out of [0,1].", name, pct));
    }
    rt.pct_of_population[n] = pct;
    pct_sum += pct;

    if (!entry.contains("out_ring_prob_by_agegrp")) {
      throw std::runtime_error(fmt::format(
          "rings: '{}' missing out_ring_prob_by_agegrp.", name));
    }
    const auto& probs = entry["out_ring_prob_by_agegrp"];
    for (const auto& [agename, val] : probs.items()) {
      Agegrp ag = agegrp_from_string(agename);
      if (ag == UNKNOWN) {
        throw std::runtime_error(fmt::format(
            "rings: '{}' has unknown agegrp '{}' in out_ring_prob_by_agegrp.",
            name, agename));
      }
      float p = val.get<float>();
      if (p < 0.0f || p > 1.0f) {
        throw std::runtime_error(fmt::format(
            "rings: '{}' agegrp '{}' out_ring_prob={} out of [0,1].",
            name, agename, p));
      }
      rt.out_ring_prob[n][static_cast<size_t>(ag.v)] = p;
    }
    // require all five real agegrps (1..5) supplied
    for (uint8_t g = 1; g < Agegrp::names.size(); ++g) {
      if (!probs.contains(Agegrp::names[g])) {
        throw std::runtime_error(fmt::format(
            "rings: '{}' missing out_ring_prob for agegrp '{}'.",
            name, Agegrp::names[g]));
      }
    }
  }

  if (!approx_equal(pct_sum, 1.0, 1e-6)) {
    throw std::runtime_error(fmt::format(
        "rings: pct_of_population values must sum to 1.0 (got {}).", pct_sum));
  }
  rt.ring_names = ring_names;
  return rt;
}

// Helper function to print infectparams
void print_infectparams(const vector<InfectParams>& infectparams, const vector<Variant> & variants) {
  fmt::println("========== InfectParams =============");
  for (size_t i = 0; i < infectparams.size(); ++i) {
    fmt::println(" ==== infectparams of variant {} ====", variants[i].show());
    fmt::print("  sendrisk={},\n  recvrisk={},\n  basemultiplier={:.2f},   halflife={}\n",
               infectparams[i].sendrisk,
               infectparams[i].recvrisk,
               infectparams[i].basemultiplier,
               infectparams[i].immunehalflife);
  }
  fmt::println("========== End InfectParams =============");
}
