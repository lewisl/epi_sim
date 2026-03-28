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
#include "setup.h"



ModelParams setup_model_params(bool dovax, string geo_path, string variants_path, string social_path, string vax_path, string vaxsched_path)
{
  // first build each needed datastructure;
  //          then wrap all of them in the aggregate initialization of the container
  GeoData geodata = load_geodata_csv(geo_path);
  auto [infectparams, progressionset, trvec, variants] =
      load_infect_params(variants_path);
  // vax related parameters don't need to be loaded if dovax == false
    VaxSet vaxdata;
    MapEnum<uint8_t> vaxlist;
    VaxSched vaxsched;
    if (dovax) {
      fmt::println("we got here to load valid vax parameters...");
      std::tie(vaxdata, vaxlist) = load_vax_data(vax_path, variants); 
      vaxsched = load_vax_sched(vaxsched_path, vaxlist);
    }
  auto socialdata = load_social_params(social_path);


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
      .vaxlist = std::move(vaxlist),
      .vaxsched = std::move(vaxsched),
  };
}

series_type build_series(int n_days, absl::CivilDay day1,
                         vector<string> series_colnames) {

  // series container
  series_type series {};
  /*
  TODO build it here
  */
  return series;
};

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
Model setup_sim(Config config,    
    const fs::path& project_dir,
    const fs::path& paramdir,
    const fs::path& geodata_fname,
    const fs::path& param_dir,
    const fs::path& variants_fname,
    const fs::path& social_fname,
    const fs::path& vax_fname,
    const fs::path& vax_sched_dir,
    const fs::path& vax_sched_fname)
{
    // Full paths to parameter files
    const string variants_path = (project_dir / param_dir / variants_fname).string();
    const string geodata_path = (project_dir / param_dir / geodata_fname).string();
    const string social_path = (project_dir / param_dir / social_fname).string();
    const string vax_path = (project_dir / param_dir / vax_fname).string();
    const string vax_sched_path = (project_dir / param_dir / vax_sched_dir / vax_sched_fname).string();

    //extract values from Config struct
      int ndays = config.days;
      int locale = config.locale;
      string date = config.calendar_start;
      bool dovax = config.dovax;


    ModelParams mp = setup_model_params(dovax, geodata_path, variants_path, social_path,
        vax_path, vax_sched_path);

    // access population of chosen locale or error
    auto locale_pos = find(mp.geodata.fips.begin(), mp.geodata.fips.end(), locale);
    if (locale_pos == mp.geodata.fips.end()) {
      throw std::runtime_error("Invalid locale input: " + std::to_string(locale) + ". Must match a locale from geodata.");
    }
    auto locale_idx = locale_pos - mp.geodata.fips.begin();
    int popn = mp.geodata.pop[locale_idx];

    PopData pop(popn, mp.vaxlist, Trait::true_false, Trait::Justint);
    auto day1 = parse_date(date);

    series_type series = build_series(ndays, day1, std::vector<std::string>{});
    vector<absl::CivilDay> caldays = build_caldays(ndays, day1);

    vector<float> indoor_seq = build_indoor_seq(ndays, locale, mp.geodata, caldays, mp.socialdata.indoor_uplift);

    return Model {
      .ndays = ndays, 
      .day1 = day1, 
      .caldays = caldays, 
      .series = series,
      .indoor_seq = indoor_seq,
      .locale = locale,
      .dovax = dovax,
      .mp = std::move(mp),
      .pop = std::move(pop)};
}
// clang-format on
