#include "parameters.cpp"

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