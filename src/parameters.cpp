#include "lib_includes.h"


#include "parameters.h"
#include "helpers.h"    // for shifter range compressor
#include <cstdint>

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
// variant data supplies infectset, progressionset, trvec, variants
//


std::tuple<vector<Variant>, vector<InfectParams>> load_variants_data(json jdata) {

  Variant::names.clear();
  vector<Variant> variants;
  variants.emplace_back(Variant{"none"});

  for (auto variant : jdata.items()) {
    variants.emplace_back(Variant{variant.key()});
  }

  if (variants.size() < 2) {
    throw std::runtime_error("No variants loaded from json file of variants. Can't run simulation.");
  }

  const Variant &primary = variants[1];


  vector<InfectParams> infectparams{};
  // Add a dummy "none" entry at index 0 to align with variants
  infectparams.emplace_back(InfectParams{});

  for (auto variant : jdata.items()) {

    auto recovery_imm_obj = variant.value()["immunity"]["recovery_immunity"];
    
    // Build the recovery_immunity vector in the correct order
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
      sendrisk = infectparams[1].sendrisk;
      std::transform(sendrisk.begin(), sendrisk.end(), sendrisk.begin(), [base](float x) {return x * base;} );
    } else {
      sendrisk = raw_sendrisk;
    }

    const auto raw_recvrisk = variant.value()["spread"]["recvrisk"].get<vector<float>>();
    vector<float> recvrisk;
    // use basemultiplier from above... 
    if (raw_recvrisk.size() == 0) {
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
  return {variants, infectparams};
}

/*
vector indexed by agegrp int of
  vector indexed by breakday of
     vector<vector<float> a matrix of progression probabilities

to parse: 
"<variant>" 
      "progression_tree"
          "<agegrp>"
              "<breakday>"
                  4 sickness conditions: "nil", "mild", "sick", "severe"
                  for each of vector<float> in [0.0, 1.0]: [recover, nil, mild, sick, severe, dead]

if no tree apply riskadjust and vaxhalflifeadjust to base
*/


std::tuple<ProgressionSet, array<float, 6>> load_progression_set(json jdata) {
  // json jdata = load_json_params(fpath);
  ProgressionSet progressionset{};

    // add empty dummy for entry 0, not used
    progressionset.progression.emplace_back(Agetree{}, ProgressionFactors{} );

  for (const auto &[variant, body] : jdata.items()) { // variant loop

    ProgressionFactors factors{};

    auto jsontree = body["progression_tree"];
    auto jsonfactors = body["progression_factors"];

    // create members of ProgressionFactors factors
    factors.riskadjust = jsonfactors["riskadjust"].get<vector<float>>();
    for (const auto &[vax, num] : jsonfactors["vaxhalflifeadjust"].get<absl::flat_hash_map<string, float>>()) {
            factors.vaxhalflifeadjust[vax] = num;
    };

    // create members of Agetree tree
    Agetree age_vec;
    if (jsontree.is_null()) {
      // Copy base tree (index 1) and apply riskadjust — base must appear first in JSON, well 2nd because nulls are at idx 0
      age_vec = progressionset.progression[1].tree;
      if (!factors.riskadjust.empty()) {          // TODO this is an error if jsontree is null!
        for (auto& breakday_map : age_vec) {
          for (auto& [day, cond_vec] : breakday_map) {
            for (auto& row : cond_vec) {
              float sum = 0.0f;
              for (float x : row) sum += x;
              if (sum != 0.0f) {
                for (size_t i = 0; i < row.size(); ++i)
                  row[i] *= factors.riskadjust[i];
                sum = 0.0f;
                for (float x : row) sum += x;
                for (float& x : row) x /= sum;  // normalize to 1.0 after multiplying times riskadjust...
              }
            }
          }
        }
      }
    } else {
      for (const auto& [age, body_age] : jsontree.items()) {
        absl::flat_hash_map<uint8_t, vector<vector<float>>> one_age_map {};
        for (const auto& [duration, body_duration] : body_age.items()) {
          vector<vector<float>> tmpvec{};
          // Explicitly load in Cond enum order: Nil(0), Mild(1), Sick(2), Severe(3)
          // nlohmann::json iterates object keys alphabetically (mild < nil < severe < sick),
          // so we MUST NOT rely on .items() iteration order here.
          for (size_t ci = 1; ci < Condition::names.size(); ++ci) {
            string key = Condition::names[ci];
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            tmpvec.push_back(body_duration[key].get<vector<float>>());
          }
          one_age_map[std::stoi(duration)] = tmpvec;
        }
        age_vec.push_back(one_age_map);
      }
    } // end null-tree branch
    
    Progression pg {
      .tree = age_vec,
      .factors = factors
      };

    progressionset.progression.push_back(pg);

  } // variant loop

  // pre-allocate small vector for performance in progression kernel function
  array<float, 6> trvec {}; 
  
  return {progressionset, trvec};
}

std::tuple<vector<InfectParams>, ProgressionSet, array<float, 6>, vector<Variant>> load_infect_params(string fpath) {
  // use one big json file for multiple output structs, etc.
  json jdata = load_json_params(fpath);

  auto [variants, infectparams] = load_variants_data(jdata);

  auto [progressionset, trvec] = load_progression_set(jdata);


  return {infectparams, progressionset, trvec, variants};
};

/*
access will look like
ProgressionSet progression{};  // assume it then gets loaded
progression[0].tree[0][5][0][0]  
    // we have 6 levels of qualifiers
    // 1) for variant "base" index = 0, 
    // 2) tree member, 
    // 3) agegrp "age0_19" index = 0, 
    // 4) breakday 5 key, 
    // 5) condition "nil" by index = 0,
    // 6) recovered probability by vector index for to recovered index = 0

*/

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

  Vax::names.clear();
  Vax::names.emplace_back("none");
  vaxset.params.clear();
  vaxset.params.emplace_back(VaxParams{});

  for (const auto &[vaxname, body] : vaxdata.items()) {  // for each vax
    const Vax vax{vaxname};
    (void)vax;

    VaxParams vx {};  // details for each vaccine
    // load items for each vax into struct
    vx.reqdshots = body["reqdshots"];
    vx.delay2ndshot = body["delay2ndshot"];
    vx.delaybooster = body["delaybooster"];
    vx.halflife = body["halflife"];
    vx.full_effect_days = body["full_effect_days"];
    vx.day1_effect = body["day1_effect"];

    // infectfactor vector
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

  json data = load_json_params(fpath);
  if (!data.contains("rings")) {
    return rt;  // rings disabled
  }

  const auto& ring_arr = data["rings"];
  if (!ring_arr.is_array()) {
    throw std::runtime_error("rings: expected an array under top-level 'rings' key.");
  }
  if (ring_arr.empty()) {
    return rt;
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
