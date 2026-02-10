#include <filesystem>

#include "parameters.h"
// #include "helpers.h"
#include "population.h"

namespace fs = std::filesystem;

/*
buildsim(ndays, locales;
    day1 = Date("2020-01-01", "yyyy-mm-dd"),    # first calendar day of simulation
    dovax = false,                              # vaccinations for people
    paramdir = "../sample_parameters",          # a directory of required parameters
    geofilename = "geo2data.csv", 
    socialfilename = "socialparams.yml",
    vaccinefilename = "vaccines.yml",
    scheddir="vaccine_schedule",
    variantfilename = "variants.yml")

    locales = locales isa Int ? [locales] : locales

    returns: 

next step: build the model with file inputs

function setup_files(ndays::Int64, locales;  # alternative: setup from complete model yaml in one file
    # must provide following inputs
    day1,
    dovax=false,
    paramdir,
    geofilename, 
    socialfilename,
    vaccinefilename,
    scheddir,
    variantfilename)

    calls: setup_model
    returns:
        model = (ndays=ndays, day1=day1, locales=locales, dat=dat, series=series, geo=geodata,
            progressionset=progressionset, dovax=dovax, vaxset=vaxset, vaxschedset=vaxschedset,
            infectset=infectset, social=socialparams, trvec=trvec, variantlist=variantlist, vaxlist=vaxlist,
            indoor_seq=indoor_seq, seriescolnames=seriescolnames)

dat consists of: popdat, agegrp_idx. inputs are: locales, geodata, n_days
    locales is a vector of locales to run during the simulation
series consists of:
*/

// for testing only
// Keep all paths as plain strings (avoid std::filesystem::path conversions).
const fs::path project_dir = fs::path(std::getenv("HOME")) / "code" / "epi_sim";
const fs::path param_dir = "sample_parameters";
const fs::path vax_sched_dir = "vaccine_100k";
const fs::path variants_fname = "variants.json";
const fs::path geodata_fname = "geo2data.csv";
const fs::path social_fname = "SocialParams.json";
const fs::path vax_fname = "vaccines.json";
const fs::path vax_sched_fname = "loc38015_old.json";

// Full paths
const string variants_path = (project_dir / param_dir / variants_fname).string();
const string geodata_path = (project_dir / param_dir / geodata_fname).string();
const string social_path = (project_dir / param_dir / social_fname).string();
const string vax_path = (project_dir / param_dir / vax_fname).string();
const string vax_sched_path = (project_dir / param_dir / vax_sched_dir / vax_sched_fname).string();


ModelParams setup_model_params(string geo_path, string variants_path, string social_path, string vax_path, string vaxsched_path)
{
  // first build each needed datastructure;
  //          then wrap all of them in the aggregate initialization of the container
  GeoData geodata = load_geodata_csv(geo_path);

  auto [infectset, progressionset, trvec, variants] = load_infect_params(variants_path);
  auto [vaxdata, vaxlist] = load_vax_data(vax_path, variants);
  auto socialdata = load_social_params(social_path);

  // Load vaxsched before moving vaxlist (since load_vax_sched needs vaxlist)
  VaxSched vaxsched = load_vax_sched(vaxsched_path, vaxlist);

  // Use aggregate initialization to construct model_params with all members at once
  // This avoids assignment to socialdata (which has const members)???
  // note the curly braces: this is initialization, NOT a call to the default constructor
  return ModelParams{
      .geodata = std::move(geodata),
      .variants = std::move(variants),
      .infectset = std::move(infectset),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = std::move(socialdata),
      .vaxset = std::move(vaxdata),
      .vaxlist = std::move(vaxlist),
      .vaxsched = std::move(vaxsched),
  };
}


std::tuple<ModelParams, PopData> setup_sim(string geo_path, string variants_path,
                                       string social_path, string vax_path,
                                       string vaxsched_path)
{
  ModelParams mp = setup_model_params(geo_path, variants_path, social_path, vax_path,
                              vaxsched_path);
  PopData pop(100, Traits::Status, Traits::Agegrp, Traits::Condition,
              mp.variants, mp.vaxlist, Traits::Vaxstatus, Traits::true_false,
              Traits::Justint);
  return {mp, pop};
}