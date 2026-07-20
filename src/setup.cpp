
#include "parameters.h"
#include "helpers.h"
#include "population.h"
#include "random.h"
#include "setup.h"

namespace fs = std::filesystem;



ModelParams setup_model_params(bool dovax, bool do_rings, string geo_path, string variants_path, string social_path, string vax_path, string vaxsched_dir, string rings_path)
{
  // first build each needed datastructure;
  //          then wrap all of them in the aggregate initialization of the container
  GeoData geodata = load_geodata_csv(geo_path);
  auto [infectparams, progressionset, trvec, variant_names] =
      load_infect_params(variants_path);
  // vax related parameters don't need to be loaded if dovax == false
    VaxSet vaxdata;
    VaxSchedSet vaxschedset;
    if (dovax) {
      fmt::println("we got here to load valid vax parameters...");
      vaxdata = load_vax_data(vax_path);
      vaxschedset = load_vax_sched_set(vaxsched_dir);
    }
  auto socialdata = load_social_params(social_path);

  RingTraits ringtraits{};
  Ring::names.clear();
  if (do_rings) {
    if (rings_path.empty()) {
      throw std::runtime_error("do_rings is true but rings is not configured.");
    }
    ringtraits = load_ring_traits(rings_path);
  }
  
  // note the curly braces: this is initialization, NOT a call to the default constructor
  return ModelParams{
      .geodata = std::move(geodata),
      .variant_names = std::move(variant_names),
      .infectparams = std::move(infectparams),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = std::move(socialdata),
      .vaxset = std::move(vaxdata),
      .vaxschedset = std::move(vaxschedset),
      .ringtraits = std::move(ringtraits),
  };
}

// Stratified ring assignment: for each agegrp, apportion that agegrp's
// people across rings using pct_of_population, then shuffle and slice.
// Preserves the population-wide age mix within each ring (subject to rounding).
// No-op when rings are disabled (pct_of_population empty or sentinel-only).
void assign_rings(PopData& pop, const std::vector<float>& pct_of_population) {
  if (pct_of_population.size() <= 1) return;

  const size_t nrings = pct_of_population.size() - 1;
  vector<float> splits(pct_of_population.begin() + 1, pct_of_population.end());

  const size_t NAGE = Agegrp::names.size();
  vector<vector<size_t>> by_age(NAGE);
  for (size_t i = 1; i <= pop.popn; ++i) {
    by_age[static_cast<size_t>(pop.agegrp[i].v)].push_back(i);
  }

  auto& gen = xo::get_gen();

  for (size_t g = 1; g < NAGE; ++g) {
    auto& idxs = by_age[g];
    if (idxs.empty()) continue;

    vector<int> counts = pop.apportion(static_cast<int>(idxs.size()), splits);
    std::shuffle(idxs.begin(), idxs.end(), gen);

    size_t cursor = 0;
    for (size_t r = 0; r < nrings; ++r) {
      const int cnt = counts[r];
      for (int k = 0; k < cnt; ++k) {
        pop.ring[idxs[cursor++]] = Ring{static_cast<uint8_t>(r + 1)};  // we can use nidx(r)
      }
    }
  }
}

// Build the per-ring membership index from the assigned ring column.
// Returns ring_members[r] = sorted-by-person-id list of 1-based person ids in ring r.
// Outer index is 1-based to match Ring ids; ring_members[0] stays empty.
// Returns an empty outer vector when ring_count == 0 (rings disabled).
std::vector<std::vector<size_t>> build_ring_members(const PopData& pop, size_t ring_count) {
  if (ring_count == 0) return {};

  std::vector<std::vector<size_t>> members(ring_count + 1);
  for (size_t i = 1; i <= pop.popn; ++i) {
    const size_t r = static_cast<size_t>(pop.ring[i].v);
    if (r == 0 || r > ring_count) continue;  // unassigned or out-of-range
    members[r].push_back(i);
  }
  return members;
}

vector <absl::CivilDay> build_caldays(int n_days, absl::CivilDay day1) {
  // vector of all calendar dates in the simulation
  vector<absl::CivilDay> caldays{};
  for (auto d = day1; d < day1 + n_days; ++d) {
    caldays.push_back(d);
  }
  return caldays;
}

vector<float> build_indoor_seq(int ndays, int locale, GeoData geodata,
                               vector<absl::CivilDay> caldays, float indoor_lift) {

  vector<float> indoor_seq(ndays, 1.0f);

  // access density factor for current locale
  auto locale_pos = find(geodata.fips.begin(), geodata.fips.end(), locale);
  if (locale_pos == geodata.fips.end()) {
    throw std::runtime_error("Invalid locale input: " + std::to_string(locale) + ". Must match a locale from geodata.");
  }
  auto locale_idx = locale_pos - geodata.fips.begin();

  absl::CivilDay indoor_st = parse_date(geodata.indoor_st[locale_idx]);
  absl::CivilDay indoor_end = parse_date(geodata.indoor_end[locale_idx]);
  int year_end = indoor_end.year();
  int year_st = indoor_st.year();

  if (year_end == year_st) {
    for (auto i = 0; i < indoor_seq.size(); ++i) {
      absl::CivilDay testdate(year_end, caldays[i].month(), caldays[i].day());
      if (testdate >= indoor_st && testdate <= indoor_end) {
        indoor_seq[i] *= indoor_lift;
      }
    }

  } else if (year_end > year_st) {

    for (auto i = 0; i < indoor_seq.size(); ++i) {
        auto set_year = (caldays[i].month() >= indoor_st.month()) ? year_st : year_end;
        absl::CivilDay testdate(set_year, caldays[i].month(), caldays[i].day());
        if (testdate >= indoor_st && testdate <= indoor_end) {
            indoor_seq[i] *= indoor_lift;
        }
      }
    } else {
    throw std::runtime_error("Invalid indoor start and end input for locale: " + std::to_string(locale) + ".");
  }

  return indoor_seq;
}

// clang-format off
// Model setup_sim(int ndays, int locale,  // require inputs
//     string date,   // all the rest have defaults...
//     bool dovax,
Model setup_sim(Config config)
{
    //extract values from Config struct
      int ndays = config.days;
      int locale = config.locale;
      string date = config.calendar_start;
      bool dovax = config.dovax;
      bool do_social_distancing = config.do_social_distancing;
      bool do_rings = config.do_rings;
      bool debug = config.debug;
      int rt_sim_interval = config.rt_sim_interval;
      std::vector<double> age_dist = config.age_dist;
      fs::path output_dir = config.output_dir;
      std::string case_label = config.case_label;
      fs::path seed = config.seed;
      fs::path social_dist = config.social_dist;

      
    ModelParams mp = setup_model_params(dovax,
        do_rings,
        config.geodata,  // convert filesystem::path objects to string
        config.variants,
        config.social_params,
        config.vaccines,
        config.vax_sched_dir,
        config.rings);

    // setup other Model members    
    vector<SeedCase> seedcases = load_seed_cases(load_json_params(seed.string()), mp);
    vector<SocialDistancing> sd_cases;  // default constructor leaves this empty
    if (do_social_distancing) {
      if (social_dist.empty()) {
        throw std::runtime_error("do_social_distancing is true but social_dist is not configured.");
      }
      sd_cases = load_sd_cases(load_json_params(social_dist), mp.socialdata); }


    // access population of chosen locale or error
    auto locale_pos = find(mp.geodata.fips.begin(), mp.geodata.fips.end(), locale);
    if (locale_pos == mp.geodata.fips.end()) {
      throw std::runtime_error("Invalid locale input: " + std::to_string(locale) + ". Must match a locale from geodata.");
    }
    auto locale_idx = locale_pos - mp.geodata.fips.begin();
    int popn = mp.geodata.pop[locale_idx];

    PopData pop(popn, age_dist);
    assign_rings(pop, mp.ringtraits.pct_of_population);
    auto ring_members = build_ring_members(pop, mp.ringtraits.ring_count());

    // precompute per-ring sizes so the spread sieve avoids touching inner-vector headers
    std::vector<size_t> ring_lengths(ring_members.size());
    for (size_t r = 0; r < ring_members.size(); ++r)
      ring_lengths[r] = ring_members[r].size();

    auto day1 = parse_date(date);

    // series_type series = build_series(ndays, day1, std::vector<std::string>{});
    vector<absl::CivilDay> caldays = build_caldays(ndays, day1);

    vector<float> indoor_seq = build_indoor_seq(ndays, locale, mp.geodata, caldays, mp.socialdata.indoor_uplift);

    return Model {
      .ndays = ndays, 
      .day1 = day1, 
      .caldays = caldays, 
      .indoor_seq = indoor_seq,
      .locale = locale,
      .dovax = dovax,
      .do_social_distancing = do_social_distancing,
      .do_rings = do_rings,
      .debug = debug,
      .rt_sim_interval = rt_sim_interval,
      .age_dist = age_dist,
      .output_dir = std::move(output_dir),
      .case_label = std::move(case_label),
      .mp = std::move(mp),
      .pop = std::move(pop),
      .ring_members = std::move(ring_members),
      .ring_lengths = std::move(ring_lengths),
      .seedcases = std::move(seedcases),
      .sd_cases = std::move(sd_cases)};
}
// clang-format on

void install_runtime_trait_names(const Model& model) {
  Variant::names = model.mp.variant_names;
  Vax::names = model.mp.vaxset.names;
  Ring::names = model.mp.ringtraits.ring_names;

  SDCase::names = {"none"};
  for (const auto& sd_case : model.sd_cases)
    SDCase::names.push_back(sd_case.name);
}