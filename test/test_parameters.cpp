#include "test_support.h"

namespace {

constexpr std::string_view GROUP = "parameters";

void test_model_params_loading(const test_support::TestRunOptions& options) {
  fmt::print("\n=== Testing ModelParams Loading ===\n\n");

  test_support::VariantNamesGuard variant_names_guard;
  test_support::VaxNamesGuard vax_names_guard;
  Variant::names.clear();
  Vax::names.clear();
  const auto paths = test_support::sample_paths();

  GeoData geodata = load_geodata_csv(paths.geodata);
  assert(geodata.num_rows > 0);
  assert(geodata.num_rows == geodata.fips.size());
  assert(geodata.num_rows == geodata.county.size());
  assert(geodata.num_rows == geodata.pop.size());
  const size_t locale_idx = test_support::require_locale_index(geodata, 38015);
  assert(geodata.county[locale_idx] == "Burleigh");
  assert(geodata.city[locale_idx] == "Bismarck");
  assert(geodata.state[locale_idx] == "ND");
  assert(geodata.pop[locale_idx] == 95626);
  assert(geodata.indoor_st[locale_idx] == "0001-09-15");
  assert(geodata.indoor_end[locale_idx] == "0002-05-30");

  auto [infectparams, progressionset, trvec, variants] = load_infect_params(paths.variants);
  assert(!variants.empty());
  assert(variants.size() == infectparams.size());
  assert(variants.size() == progressionset.progression.size());
  assert(variants[0].show() == "none");
  assert(variants[1].show() == "base");
  assert(variants[2].show() == "alpha");
  assert(infectparams[1].sendrisk.size() > 20);
  assert(approx_equal(infectparams[1].sendrisk[1], 0.0, 1e-9));
  assert(approx_equal(infectparams[1].sendrisk[2], 0.3, 1e-9));
  assert(approx_equal(infectparams[1].sendrisk[5], 0.85, 1e-9));
  assert(approx_equal(infectparams[1].recvrisk[0], 0.1, 1e-9));
  assert(approx_equal(infectparams[1].recvrisk[4], 0.56, 1e-9));
  assert(progressionset.progression[1].tree.size() == 5);
  assert(progressionset.progression[1].tree[0].contains(5));
  const auto& base_age0_day5_nil = progressionset.progression[1].tree[0].at(5)[0];
  assert(base_age0_day5_nil.size() == 6);
  assert(approx_equal(base_age0_day5_nil[1], 0.4, 1e-9));
  assert(approx_equal(base_age0_day5_nil[2], 0.5, 1e-9));
  assert(progressionset.progression[2].factors.riskadjust.size() == 6);
  assert(approx_equal(progressionset.progression[2].factors.riskadjust[3], 1.1, 1e-9));
  assert(std::all_of(trvec.begin(), trvec.end(),
                     [](float value) { return approx_equal(value, 0.0, 1e-9); }));

  SocialParams socialdata = load_social_params(paths.social);
  assert(approx_equal(socialdata.gammashape, 1.0, 1e-9));
  assert(approx_equal(socialdata.indoor_uplift, 1.1, 1e-9));
  assert(approx_equal(socialdata.contactfactors[0][1], 1.995, 1e-9));
  assert(approx_equal(socialdata.contactfactors[3][4], 0.475, 1e-9));
  assert(approx_equal(socialdata.touchfactors[0][0], 0.55, 1e-9));
  assert(approx_equal(socialdata.touchfactors[4][0], 0.28, 1e-9));

  VaxSet vaxset = load_vax_data(paths.vaccines);
  assert(vaxset.size() == 3);
  assert(Vax::names.size() == 4);
  assert(Vax::names[0] == "none");
  assert(Vax::names[1] == "Pfizer");
  assert(Vax::names[2] == "Moderna");
  assert(Vax::names[3] == "JnJ");
  const auto& pfizer = test_support::require_vax(vaxset, "Pfizer");
  assert(pfizer.reqdshots == 2);
  assert(pfizer.delay2ndshot == 21);
  assert(pfizer.delaybooster == 160);
  assert(approx_equal(pfizer.day1_effect, 0.65, 1e-9));
  assert(approx_equal(test_support::require_named_factor(pfizer.infectfactor, "delta"), 0.85, 1e-9));
  assert(pfizer.effectiveness.size() == 3);
  assert(pfizer.effectiveness[1].first == "full");
  assert(approx_equal(test_support::require_named_factor(pfizer.effectiveness[1].second, "delta"), 0.8, 1e-9));

  VaxSchedSet vaxschedset = load_vax_sched_set(paths.vax_sched_dir);
  assert(vaxschedset.size() == 2);

  const auto& old_sched = test_support::require_sched(vaxschedset, "loc38015_old");
  assert(old_sched.vaxesincluded.size() == 3);
  const auto& jnj_sched = test_support::require_sched_vax(old_sched, "JnJ");
  assert(approx_equal(jnj_sched.mix, 0.05, 1e-9));
  assert(jnj_sched.starting_doses == 4000);
  assert(approx_equal(jnj_sched.pct2ndshot, 0.0, 1e-9));
  assert(approx_equal(jnj_sched.pctboost, 0.4, 1e-9));
  assert(old_sched.dayrange.first == 350);
  assert(old_sched.dayrange.second == 700);
  assert(approx_equal(old_sched.targetpct, 0.95, 1e-9));
  assert(old_sched.filtervec.size() == 2);
  assert(old_sched.filtervec[0] == AGE80_UP);
  assert(old_sched.filtervec[1] == AGE60_79);
  assert(old_sched.shotmode == "all");
  assert(old_sched.pattern.size() == 12);
  assert(approx_equal(old_sched.pattern[3], 0.1, 1e-9));
  assert(static_cast<bool>(old_sched.spreadfunc));

  const auto& young_sched = test_support::require_sched(vaxschedset, "loc38015_young");
  assert(approx_equal(young_sched.targetpct, 0.65, 1e-9));
  assert(young_sched.filtervec.size() == 3);
  assert(young_sched.filtervec[0] == AGE0_19);
  assert(young_sched.filtervec[1] == AGE20_39);
  assert(young_sched.filtervec[2] == AGE40_59);
  assert(static_cast<bool>(young_sched.spreadfunc));

  if (options.write_artifacts) {
    std::ostringstream artifact;
    artifact << "Parameter loader summary\n";
    artifact << "========================\n\n";
    artifact << "geodata:\n";
    artifact << "  rows: " << geodata.num_rows << "\n";
    artifact << "  locale 38015 county/city/state: "
             << geodata.county[locale_idx] << ", "
             << geodata.city[locale_idx] << ", "
             << geodata.state[locale_idx] << "\n";
    artifact << "  locale 38015 pop: " << geodata.pop[locale_idx] << "\n";
    artifact << "  indoor window: " << geodata.indoor_st[locale_idx]
             << " -> " << geodata.indoor_end[locale_idx] << "\n\n";

    artifact << "variants:\n";
    artifact << fmt::format("  names: [{}]\n", fmt::join(Variant::names, ", "));
    artifact << "  infectparams count: " << infectparams.size() << "\n";
    artifact << "  base sendrisk[2]: " << infectparams[1].sendrisk[2] << "\n";
    artifact << "  alpha riskadjust[3]: " << progressionset.progression[2].factors.riskadjust[3] << "\n\n";

    artifact << "social:\n";
    artifact << "  gammashape: " << socialdata.gammashape << "\n";
    artifact << "  indoor_uplift: " << socialdata.indoor_uplift << "\n";
    artifact << "  contactfactors[0][1]: " << socialdata.contactfactors[0][1] << "\n";
    artifact << "  touchfactors[4][0]: " << socialdata.touchfactors[4][0] << "\n\n";

    artifact << "vaccines:\n";
    artifact << fmt::format("  names: [{}]\n", fmt::join(Vax::names, ", "));
    artifact << "  set size: " << vaxset.size() << "\n";
    artifact << "  Pfizer reqdshots/delay2nd/booster: "
             << pfizer.reqdshots << "/" << pfizer.delay2ndshot << "/"
             << pfizer.delaybooster << "\n";
    artifact << "  Pfizer delta infectfactor: "
             << test_support::require_named_factor(pfizer.infectfactor, "delta") << "\n\n";

    artifact << "schedules:\n";
    artifact << "  schedule count: " << vaxschedset.size() << "\n";
    artifact << "  loc38015_old targetpct/dayrange: "
             << old_sched.targetpct << " / [" << old_sched.dayrange.first
             << ", " << old_sched.dayrange.second << "]\n";
    artifact << "  loc38015_young targetpct/filter count: "
             << young_sched.targetpct << " / " << young_sched.filtervec.size() << "\n\n";

    artifact << "assembled ModelParams inputs:\n";
    artifact << "  variants: " << variants.size() << "\n";
    artifact << "  vaxset: " << vaxset.size() << "\n";
    artifact << "  schedules: " << vaxschedset.size() << "\n";

    test_support::write_artifact_text(options, GROUP, "loader_summary.txt", artifact.str());
  }

  ModelParams mp{
      .geodata = std::move(geodata),
      .variants = std::move(variants),
      .infectparams = std::move(infectparams),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = std::move(socialdata),
      .vaxset = std::move(vaxset),
      .vaxschedset = std::move(vaxschedset),
  };

  assert(mp.geodata.pop[locale_idx] == 95626);
  assert(mp.variants[1].show() == "base");
  assert(mp.vaxset.size() == 3);
  assert(mp.vaxschedset.size() == 2);

  fmt::println("=== ModelParams Loading Test Completed ===");
}

}  // namespace

void run_parameter_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running parameter tests...");
  test_model_params_loading(options);
  if (options.write_artifacts) {
    fmt::println("parameter artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("parameter tests passed.");
}