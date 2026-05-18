/*
Overall TODO
- setup history series including 
- with history series struct have caldays: simulation ordinal days converted to CivilDays
- for simulation, create indoor_seq:  pre-calculate which days get the indoor uplift on transmission
- turn runtime enum into a template
- add status filtering to seeding
*/

#include "../src/lib_includes.h"

#include "parameters.h"
#include "helpers.h"
#include "population.h"
#include "random.h"
#include "setup.h"



ModelParams setup_model_params(bool dovax, string geo_path, string variants_path, string social_path, string vax_path, string vaxsched_dir, string rings_path)
{
  // first build each needed datastructure;
  //          then wrap all of them in the aggregate initialization of the container
  GeoData geodata = load_geodata_csv(geo_path);
  auto [infectparams, progressionset, trvec, variants] =
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
  if (!rings_path.empty()) {
    ringtraits = load_ring_traits(rings_path);
  }

  // Use aggregate initialization to construct model_params with all members at once
  // note the curly braces: this is initialization, NOT a call to the default constructor
  return ModelParams{
      .geodata = std::move(geodata),
      .variants = std::move(variants),
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
        pop.ring[idxs[cursor++]] = Ring{static_cast<uint8_t>(r + 1)};
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
      bool debug = config.debug;

    ModelParams mp = setup_model_params(dovax,
        config.geodata.string(),  // convert filesystem::path objects to string
        config.variants.string(),
        config.social.string(),
        config.vaccines.string(),
        config.vax_sched_dir.string(),
        config.rings.string());

    // access population of chosen locale or error
    auto locale_pos = find(mp.geodata.fips.begin(), mp.geodata.fips.end(), locale);
    if (locale_pos == mp.geodata.fips.end()) {
      throw std::runtime_error("Invalid locale input: " + std::to_string(locale) + ". Must match a locale from geodata.");
    }
    auto locale_idx = locale_pos - mp.geodata.fips.begin();
    int popn = mp.geodata.pop[locale_idx];

    PopData pop(popn);
    assign_rings(pop, mp.ringtraits.pct_of_population);
    auto ring_members = build_ring_members(pop, mp.ringtraits.ring_count());
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
      .debug = debug,
      .mp = std::move(mp),
      .pop = std::move(pop),
      .ring_members = std::move(ring_members)};
}
// clang-format on
