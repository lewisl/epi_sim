#include <filesystem>

#include "parameters.h"
// #include "helpers.h"
#include "population.h"

namespace fs = std::filesystem;


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


ModelParams load_model_params(string geo_path, string variants_path, string social_path, string vax_path, string vaxsched_path)
{
  // first build each needed datastructure;
  //          then wrap all of them in the aggregate initialization of the container
  GeoData geodata = load_geodata_csv(geo_path);

  auto [infectset, progressionset, trvec, variants] = load_infect_params(variants_path);
  auto [vaxdata, vaxlist] = load_vax_data(vax_path, variants);

  // Load vaxsched before moving vaxlist (since load_vax_sched needs vaxlist)
  VaxSched vaxsched = load_vax_sched(vaxsched_path, vaxlist);

  // Use aggregate initialization to construct model_params with all members at once
  // This avoids assignment to socialdata (which has const members)
  // note the curly braces: this is initialization, NOT a call to the default constructor
  return ModelParams{
      .geodata = std::move(geodata),
      .variants = std::move(variants),
      .infectset = std::move(infectset),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = load_social_params(social_path),
      .vaxset = std::move(vaxdata),
      .vaxlist = std::move(vaxlist),
      .vaxsched = std::move(vaxsched),
  };
}


std::tuple<ModelParams, PopData> setup(string geo_path, string variants_path,
                                       string social_path, string vax_path,
                                       string vaxsched_path)
{
  auto mp = load_model_params(geo_path, variants_path, social_path, vax_path,
                              vaxsched_path);
  PopData pop(100, traits::Status, traits::Agegrp, traits::Condition,
              mp.variants, mp.vaxlist, traits::Vaxstatus, traits::true_false,
              traits::Justint);
  return {mp, pop};
}