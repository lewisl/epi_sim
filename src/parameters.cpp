#include "lib_includes.h"


#include "parameters.h"
#include "helpers.h"    // for shifter range compressor

// using json = nlohmann::json;
using json = nlohmann::ordered_json;
// using std::array;
using std::string;
using std::vector;



json load_json_params(string fpath) {

  try {
    std::ifstream fcontent(fpath);
    json data = json::parse(fcontent);

    return data;
  }

  catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
      return json();  // empty object
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

  vector<Variant> variants;
  variants.emplace_back(Variant{0});
  Variant::names.push_back("none");

  uint8_t vnum{1};
  for (auto variant : jdata.items()) {
    variants.emplace_back(Variant{vnum});
    Variant::names.push_back(variant.key());
    ++vnum;
    // variants.add_item(variant.key());
  }

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

    infectparams.emplace_back(InfectParams{
        .sendrisk = variant.value()["spread"]["sendrisk"],
        .recvrisk = variant.value()["spread"]["recvrisk"],
        .recovery_immunity = std::move(recovery_immunity),
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
    vector<absl::flat_hash_map<uint8_t, vector<vector<float>>>> age_vec{};
    for (const auto& [age, body_age] : jsontree.items()) {  // age is a string we won't store because it will be the vector index
      absl::flat_hash_map<uint8_t, vector<vector<float>>> one_age_map {};
      for (const auto& [duration, body_duration] : body_age.items()) {
        vector<vector<float>> tmpvec{};
        for (const auto &[cond, probvec] : body_duration.items()) {  // cond is a string won't be stored because it will be the vect idx
          tmpvec.push_back(probvec);
        }
        one_age_map[std::stoi(duration)] = tmpvec;
      };
      age_vec.push_back(one_age_map);
    }; // age loop
    
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



std::tuple<VaxSet, RuntimeEnum> load_vax_data(string fpath, vector<Variant> variants) {
  VaxSet vaxset{};
  RuntimeEnum vaxlist = RuntimeEnum();

  json vaxdata = load_json_params(fpath); // read data from json file input

  vector<std::pair<string, VaxParams>> vaxvec{}; // outer container
                                                 // 
  for (const auto &[vaxname, body] : vaxdata.items()) {  // for each vax
    vaxlist.add_item(vaxname);

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
      if (body["infectfactor"].contains(variantname))
        vx.infectfactor.emplace_back(variantname, body["infectfactor"][variantname]);
      else
        fmt::println("\nWARNING: variant effectiveness missing for vax: {} variant {}\n",
            vaxname, variantname);
    }

    // effectiveness vector of vector
    for (const auto &shot : vaxset.shot_types.names) {
      vector<std::pair<string, float>> variant_effectiveness {};
      for (const auto &variantname : Variant::names) {
        if (body["effectiveness"][shot].contains(variantname))
          variant_effectiveness.emplace_back(variantname, body["effectiveness"][shot][variantname]);
        else 
          fmt::println("\nWARNING: variant effectiveness missing for vax: {} shot {} variant {}\n",
              vaxname, shot, variantname);
      };
      vx.effectiveness.emplace_back(shot, variant_effectiveness);
    }
    vaxvec.emplace_back(vaxname, vx);
  }

  vaxset.vaxset = vaxvec;  // Assign the populated vector to vaxset
  return {vaxset, vaxlist};
};



VaxSched load_vax_sched(const string &fname, RuntimeEnum vaxlist) {
  json jdata = load_json_params(fname);
  VaxSched sched{};

  //  vaxesincluded member
  for (const auto &[vax, factors] : jdata["vaxesincluded"].items()) {
    PerVaxSpec spec{};
    if (vaxlist.lookup.find(vax) == vaxlist.lookup.end())
      fmt::println("WARNING: Vaccine {} not found in vaxlist.", vax);
    else {
      spec.vax_name = vax;  // Store the vaccine name
      spec.mix = factors["mix"];
      spec.starting_doses = factors["starting_doses"];
      spec.pct2ndshot = factors["pct2ndshot"];
      spec.pctboost = factors["pctboost"];
      spec.alternate = factors["alternate"];
      sched.vaxesincluded.push_back(spec);
    }
  };
  // other members
  sched.dayrange = {jdata["dayrange"][0], jdata["dayrange"][1]}; // vector of 2 set to pair
  sched.targetpct = jdata["targetpct"];
  sched.filtervec = jdata["filtervec"];
  sched.shotmode = jdata["shotmode"];
  sched.pattern.assign(jdata["pattern"].begin(), jdata["pattern"].end());
  // Handle null spreadfunc
  if (!jdata["spreadfunc"].is_null()) {
    sched.spreadfunc = jdata["spreadfunc"];
  }

  return sched;
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

// Helper function to print infectparams
void print_infectparams(const vector<InfectParams>& infectparams, const vector<Variant> & variants) {
  fmt::println("========== InfectParams =============");
  for (size_t i = 0; i < infectparams.size(); ++i) {
    fmt::println(" ==== infectparams of variant {} ====", variants[i].name());
    fmt::print("  sendrisk={},\n  recvrisk={},\n  base={:.2f},   halflife={}\n",
               infectparams[i].sendrisk,
               infectparams[i].recvrisk,
               infectparams[i].basemultiplier,
               infectparams[i].immunehalflife);
  }
  fmt::println("========== End InfectParams =============");
}
