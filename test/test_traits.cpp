#include "test_support.h"

namespace {

constexpr std::string_view GROUP = "traits";

template <typename Hist>
void check_scalar_history_overflow(Hist& history) {
  for (int day = 1; day <= 17; ++day) history.set(static_cast<int16_t>(day));

  CHECK(history.count == 17);
  CHECK(history.stored_count() == 16);
  CHECK(history.arr[0] == 2);
  CHECK(history.arr[15] == 17);
  CHECK(history.latest() == 17);
  CHECK(history.show() == "2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17");
}

template <typename Hist>
void check_scalar_history_empty(const Hist& history) {
  CHECK(history.count == 0);
  CHECK(history.stored_count() == 0);
  CHECK(history.latest() == 0);
  CHECK(history.show() == "");
}

void test_primitive_wrappers() {
  test_support::RingNamesGuard ring_guard;
  Ring::names.clear();

  Duration duration;
  Ring ring;
  Sickday sickday;
  Recovday recovday;
  Deadday deadday;
  Testday testday;
  Quarday quarday;
  Vaxday vaxday;

  CHECK(duration == 0);
  CHECK(ring.v == 0);
  CHECK(sickday == 0);

  duration = 5;
  ring = 3;
  sickday = 12;
  recovday = 14;
  deadday = 21;
  testday = 8;
  quarday = 9;
  vaxday = 17;

  CHECK(duration.show() == "5");
  CHECK(ring.show() == "3");
  CHECK(sickday.show() == "12");
  CHECK(recovday.show() == "14");
  CHECK(deadday.show() == "21");
  CHECK(testday.show() == "8");
  CHECK(quarday.show() == "9");
  CHECK(vaxday.show() == "17");

  CHECK(duration < 6);
  CHECK(ring.v < 4);
  CHECK(4 < duration);
  Duration next = duration + 2;
  CHECK(next == 7);
  next += 3;
  CHECK(next == 10);
  const Duration before_post = next++;
  CHECK(before_post == 10);
  CHECK(next == 11);
  ++next;
  CHECK(next == 12);
}

void test_wrapper_comparisons() {
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"error", "first", "second"};
  CHECK(Duration{5} < Duration{6});
  CHECK(Ring{1} < Ring{2});
  CHECK(Sickday{3} < Sickday{4});
  CHECK(Recovday{5} < Recovday{6});
  CHECK(Deadday{7} < Deadday{8});
  CHECK(Testday{9} < Testday{10});
  CHECK(Quarday{11} < Quarday{12});
  CHECK(Vaxday{13} < Vaxday{14});
}

void test_compile_time_trait_names_and_lookup() {
  CHECK(UNKNOWN.show() == "unknown");
  CHECK(AGE20_39.show() == "age20_39");
  CHECK(UNEXPOSED.show() == "unexposed");
  CHECK(RECOVERED.show() == "recovered");
  CHECK(UNINFECTED.show() == "uninfected");
  CHECK(SEVERE.show() == "severe");
  CHECK(Vaxstat::booster.show() == "booster");
  CHECK(magic_enum::enum_name(Progressmap::ToDead) == "ToDead");

  CHECK(trait_from_string<Agegrp>("AGE20_39") == AGE20_39);
  CHECK(trait_from_string<Status>("infectious") == INFECTIOUS);
  CHECK(trait_from_string<Condition>("Mild") == MILD);
  CHECK(trait_from_string<Vaxstatus>("full") == Vaxstat::full);
  CHECK(!trait_from_string<Agegrp>("bad_age").has_value());
  CHECK(!trait_from_string<Status>("bad_status").has_value());
  CHECK(!trait_from_string<Condition>("bad_condition").has_value());
  CHECK(!trait_from_string<Vaxstatus>("bad_vaxstatus").has_value());
}

template <typename T>
void check_fixed_trait_metadata() {
  static_assert(sizeof(T) == sizeof(uint8_t));
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(!std::is_convertible_v<int, T>);
  for (size_t i = 0; i < T::names.size(); ++i) {
    const T wrapped{i};
    CHECK(wrapped.v == i);
    CHECK(wrapped.show() == T::names[i]);
    CHECK(trait_from_string<T>(T::names[i]) == wrapped);
    std::string uppercase{T::names[i]};
    std::ranges::transform(uppercase, uppercase.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    CHECK(trait_from_string<T>(uppercase) == wrapped);
  }
  CHECK(!trait_from_string<T>("").has_value());
  CHECK(!trait_from_string<T>("COUNT").has_value());
}

void test_fixed_metadata_round_trips() {
  validate_fixed_trait_names();
  check_fixed_trait_metadata<Agegrp>();
  check_fixed_trait_metadata<Status>();
  check_fixed_trait_metadata<Condition>();
  check_fixed_trait_metadata<Vaxstatus>();

  CHECK(UNKNOWN.v == 0 && AGE0_19.v == 1 && AGE20_39.v == 2
        && AGE40_59.v == 3 && AGE60_79.v == 4 && AGE80_UP.v == 5);
  CHECK(NONE.v == 0 && UNEXPOSED.v == 1 && INFECTIOUS.v == 2
        && RECOVERED.v == 3 && DEAD.v == 4);
  CHECK(UNINFECTED.v == 1 && NIL.v == 2 && MILD.v == 3
        && SICK.v == 4 && SEVERE.v == 5);
  CHECK(Vaxstat::none.v == 0 && Vaxstat::first.v == 1 && Vaxstat::full.v == 2
        && Vaxstat::booster.v == 3 && Vaxstat::exhausted.v == 4);

  constexpr std::array<std::string_view, 6> outcomes{
      "ToRecover", "ToNil", "ToMild", "ToSick", "ToSevere", "ToDead"};
  CHECK(magic_enum::enum_names<Progressionmap>() == outcomes);
  for (size_t i = 0; i < outcomes.size(); ++i) {
    const auto value = magic_enum::enum_values<Progressionmap>()[i];
    CHECK(std::to_underlying(value) == i);
    CHECK(trait_from_string<Progressionmap>(std::string{outcomes[i]}) == value);
  }
  CHECK(trait_from_string<Progressionmap>("torecover") == Progressmap::ToRecover);
  CHECK(!trait_from_string<Progressionmap>("missing").has_value());
  CHECK(!trait_from_string<Agegrp>("age80up").has_value());
}

// Agegrp is the primary comorbidity marker for covid and is heavily used
// throughout the sim, so its string-construction path (resolve_name, distinct
// from the generic trait_from_string<T> lookup) gets full round-trip coverage.
void test_agegrp_string_constructor_round_trips_all_names() {
  CHECK(Agegrp("unknown")  == UNKNOWN);
  CHECK(Agegrp("age0_19")  == AGE0_19);
  CHECK(Agegrp("age20_39") == AGE20_39);
  CHECK(Agegrp("age40_59") == AGE40_59);
  CHECK(Agegrp("age60_79") == AGE60_79);
  CHECK(Agegrp("age80_up") == AGE80_UP);

  // resolve_name lowercases its input before matching
  CHECK(Agegrp("AGE0_19")  == AGE0_19);
  CHECK(Agegrp("Age80_Up") == AGE80_UP);

  bool threw = false;
  try { Agegrp invalid{"not_a_group"}; }
  catch (const std::invalid_argument&) { threw = true; }
  CHECK(threw);
}

// Status and Condition share Agegrp's resolve_name-based string constructor
// pattern; confirm it holds for them too (lighter coverage -- these columns
// see far less direct string-construction use in the sim than Agegrp does).
void test_status_and_condition_string_constructors() {
  CHECK(Status("unexposed")  == UNEXPOSED);
  CHECK(Status("INFECTIOUS") == INFECTIOUS);
  CHECK(Status("Recovered")  == RECOVERED);
  CHECK(Status("not_a_status") == NONE);

  CHECK(Condition("nil")    == NIL);
  CHECK(Condition("SEVERE") == SEVERE);
  CHECK(Condition("Mild")   == MILD);
  CHECK(Condition("not_a_condition") == UNINFECTED);
}

template <typename Exception, typename F>
void check_constructor_throws(F action) {
  bool threw = false;
  try { action(); }
  catch (const Exception&) { threw = true; }
  CHECK(threw);
}

template <typename T>
void check_runtime_trait_construction(size_t first_valid = 0) {
  static_assert(!std::is_convertible_v<int, T>);
  static_assert(!std::is_constructible_v<T, Agegrp>);
  static_assert(!std::is_constructible_v<T, TraitID>);
  const auto saved_names = T::names;
  CHECK(T{}.v == 0);
  for (size_t i = first_valid; i < T::names.size(); ++i) {
    CHECK(T{i}.v == i);
    CHECK(T{std::string_view{T::names[i]}} == T{i});
  }
  check_constructor_throws<std::out_of_range>([] { (void)T{-1}; });
  check_constructor_throws<std::out_of_range>([] { (void)T{T::names.size()}; });
  check_constructor_throws<std::out_of_range>([] { (void)T{uint64_t{1} << 32}; });
  check_constructor_throws<std::invalid_argument>([] { (void)T{"missing_trait_name"}; });
  CHECK(T::names == saved_names);
}

void test_runtime_traits_resolve_existing_names() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::SDCaseNamesGuard sdcase_guard;

  Variant::names = {"none", "alpha", "delta"};
  Vax::names = {"none", "Pfizer"};
  SDCase::names = {"none", "distancing"};

  const Variant none_variant{"none"};
  const Variant alpha{"alpha"};
  const Variant delta{"delta"};

  const Vax none_vax{"none"};
  const Vax pfizer{"Pfizer"};

  const SDCase none_sd{"none"};
  const SDCase distancing{"distancing"};

  CHECK(none_variant.show() == "none");
  CHECK(alpha.show() == "alpha");
  CHECK(delta.show() == "delta");
  CHECK(Variant::names.size() == 3);

  CHECK(none_vax.show() == "none");
  CHECK(pfizer.show() == "Pfizer");
  CHECK(Vax::names.size() == 2);

  CHECK(none_sd.show() == "none");
  CHECK(distancing.show() == "distancing");
  CHECK(SDCase::names.size() == 2);
  check_runtime_trait_construction<Variant>();
  check_runtime_trait_construction<Vax>();
  check_runtime_trait_construction<SDCase>();
}

void test_ring_resolves_existing_names() {
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"error", "Jail", "School"};

  const Ring jail{"Jail"};
  const Ring school{"School"};

  CHECK(Ring::names.size() == 3);
  CHECK(jail.v == 1);
  CHECK(school.v == 2);
  CHECK(jail.show() == "Jail");
  CHECK(school.show() == "School");
  CHECK(trait_from_string<Ring>("sChOoL") == school);
  CHECK(trait_from_string<Ring>("error") == Ring{});
  CHECK(!trait_from_string<Ring>("missing_ring").has_value());
  check_runtime_trait_construction<Ring>(1);
  check_constructor_throws<std::out_of_range>([] { (void)Ring{0}; });
  check_constructor_throws<std::invalid_argument>([] { (void)Ring{"error"}; });
}

void test_runtime_histories_render_values() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "alpha", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};

  VariantHist variant_hist;
  variant_hist.set(Variant{1});
  variant_hist.set(Variant{2});
  CHECK(variant_hist.count == 2);
  CHECK(variant_hist.latest() == Variant{2});
  CHECK(variant_hist.show() == "alpha|delta");

  VaxHist vax_hist;
  vax_hist.set(Vax{1});
  vax_hist.set(Vax{2});
  CHECK(vax_hist.count == 2);
  CHECK(vax_hist.latest() == Vax{2});
  CHECK(vax_hist.show() == "pfizer|moderna");
}

void test_runtime_trait_lookup() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::SDCaseNamesGuard sdcase_guard;
  Variant::names = {"none", "alpha", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};
  SDCase::names = {"none", "distancing"};

  CHECK(trait_from_string<Variant>("DELTA") == Variant{2});
  CHECK(trait_from_string<Vax>("Moderna") == Vax{2});
  CHECK(trait_from_string<SDCase>("distancing") == SDCase{1});
  CHECK(!trait_from_string<Variant>("missing_variant").has_value());
  CHECK(!trait_from_string<Vax>("missing_vax").has_value());
  CHECK(!trait_from_string<SDCase>("missing_sdcase").has_value());
}

void test_histories_empty_state() {
  VariantHist variant_hist;
  VaxHist vax_hist;
  SickdayHist sickday_hist;
  RecovdayHist recovday_hist;
  TestdayHist testday_hist;
  VaxdayHist vaxday_hist;

  CHECK(variant_hist.count == 0);
  CHECK(variant_hist.stored_count() == 0);
  CHECK(variant_hist.latest() == Variant{});
  CHECK(variant_hist.show() == "");

  CHECK(vax_hist.count == 0);
  CHECK(vax_hist.stored_count() == 0);
  CHECK(vax_hist.latest() == Vax{});
  CHECK(vax_hist.show() == "");

  check_scalar_history_empty(sickday_hist);
  check_scalar_history_empty(recovday_hist);
  check_scalar_history_empty(testday_hist);
  check_scalar_history_empty(vaxday_hist);
}

void test_runtime_histories_overflow() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "variant_1", "variant_2", "variant_3"};
  Vax::names = {"none", "vax_1", "vax_2", "vax_3"};

  VariantHist variant_hist;
  VaxHist vax_hist;
  for (int i = 1; i <= 17; ++i) {
    variant_hist.set(Variant{(i % 3) + 1});
    vax_hist.set(Vax{(i % 3) + 1});
  }

  CHECK(variant_hist.count == 17);
  CHECK(variant_hist.stored_count() == 16);
  CHECK(variant_hist.arr[0] == Variant{3});
  CHECK(variant_hist.arr[15] == Variant{3});
  CHECK(variant_hist.latest() == Variant{3});
  CHECK(variant_hist.show() ==
        "variant_3|variant_1|variant_2|variant_3|variant_1|variant_2|variant_3|variant_1|"
        "variant_2|variant_3|variant_1|variant_2|variant_3|variant_1|variant_2|variant_3");

  CHECK(vax_hist.count == 17);
  CHECK(vax_hist.stored_count() == 16);
  CHECK(vax_hist.arr[0] == Vax{3});
  CHECK(vax_hist.arr[15] == Vax{3});
  CHECK(vax_hist.latest() == Vax{3});
  CHECK(vax_hist.show() ==
        "vax_3|vax_1|vax_2|vax_3|vax_1|vax_2|vax_3|vax_1|vax_2|vax_3|vax_1|"
        "vax_2|vax_3|vax_1|vax_2|vax_3");
}

void test_scalar_histories_overflow() {
  SickdayHist sickday_hist;
  RecovdayHist recovday_hist;
  TestdayHist testday_hist;
  VaxdayHist vaxday_hist;

  check_scalar_history_overflow(sickday_hist);
  check_scalar_history_overflow(recovday_hist);
  check_scalar_history_overflow(testday_hist);
  check_scalar_history_overflow(vaxday_hist);
}

void write_traits_artifact(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::SDCaseNamesGuard sdcase_guard;

  std::ostringstream artifact;
  artifact << "Trait summary\n";
  artifact << "============\n\n";

  artifact << fmt::format("Agegrp names: [{}]\n", fmt::join(Agegrp::names, ", "));
  artifact << fmt::format("Status names: [{}]\n", fmt::join(Status::names, ", "));
  artifact << fmt::format("Condition names: [{}]\n", fmt::join(Condition::names, ", "));
  artifact << fmt::format("Vaxstatus names: [{}]\n\n", fmt::join(Vaxstatus::names, ", "));

  artifact << "trait_from_string examples:\n";
  artifact << "  AGE20_39 -> " << trait_from_string<Agegrp>("AGE20_39")->show() << "\n";
  artifact << "  infectious -> " << trait_from_string<Status>("infectious")->show() << "\n";
  artifact << "  Mild -> " << trait_from_string<Condition>("Mild")->show() << "\n";
  artifact << "  full -> " << trait_from_string<Vaxstatus>("full")->show() << "\n\n";

  Variant::names = {"none", "alpha", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};
  SDCase::names = {"none", "distancing"};
  artifact << fmt::format("Variant names: [{}]\n", fmt::join(Variant::names, ", "));
  artifact << fmt::format("Vax names: [{}]\n", fmt::join(Vax::names, ", "));
  artifact << fmt::format("SDCase names: [{}]\n\n", fmt::join(SDCase::names, ", "));

  VariantHist variant_hist;
  variant_hist.set(Variant{1});
  variant_hist.set(Variant{2});
  VaxHist vax_hist;
  vax_hist.set(Vax{1});
  vax_hist.set(Vax{2});
  artifact << "rendered histories:\n";
  artifact << "  VariantHist: " << variant_hist.show() << "\n";
  artifact << "  VaxHist: " << vax_hist.show() << "\n";

  SickdayHist sickday_hist;
  for (int day = 1; day <= 17; ++day) sickday_hist.set(static_cast<int16_t>(day));
  artifact << "  SickdayHist overflow: " << sickday_hist.show() << "\n";
  artifact << "  SickdayHist latest/stored: " << sickday_hist.latest() << "/"
           << sickday_hist.stored_count() << "\n";

  test_support::write_artifact_text(options, GROUP, "trait_summary.txt", artifact.str());
}


//
// PopData initialization: every person starts in the documented zero/default state,
// index 0 is unused, and age groups follow the apportioned age_dist.
//
void test_popdata_initial_state() {
  test_support::RuntimeNamesGuard guard;
  test_support::install_names({"base"});
  const vector<double> age_dist{0.251, 0.271, 0.255, 0.184, 0.039};
  PopData pop(1000, age_dist);

  CHECK_EQ(pop.popz, pop.popn + 1);
  CHECK_EQ(pop.status.size(), pop.popz);
  CHECK_EQ(pop.vaxday_hist.size(), pop.popz);
  CHECK_EQ(pop.all_idx.size(), pop.popn);
  CHECK_EQ(pop.all_idx.front(), size_t{1});
  CHECK_EQ(pop.all_idx.back(), pop.popn);
  CHECK_EQ(pop.agegrp[0], UNKNOWN);

  std::array<int, 6> ages{};
  int bad = 0;
  for (size_t p = 1; p <= pop.popn; ++p) {
    ++ages[pop.agegrp[p].v];
    bad += pop.status[p] != UNEXPOSED || pop.cond[p] != UNINFECTED || pop.duration[p] != 0 ||
           pop.variant[p].v != 0 || pop.ring[p].v != 1 || pop.sdcase[p].v != 0 ||
           pop.vaxstatus[p] != Vaxstat::none || pop.vax[p].v != 0 || pop.sickday[p] != 0 ||
           pop.variant_hist[p].count != 0 || pop.quar[p] != 0;
  }
  CHECK_MSG(bad == 0, "{} people not in the default initial state", bad);
  CHECK_EQ(ages[0], 0);
  int sum = 0;
  for (size_t a = 1; a <= 5; ++a) {
    CHECK_EQ(ages[a], pop.agegrp_parts[a - 1]);
    CHECK_NEAR(ages[a], 1000 * age_dist[a - 1], 1.0);
    sum += ages[a];
  }
  CHECK_EQ(sum, 1000);
}

// PopData defaults every person to Ring{1}, so the Ring registry must exist first.
void test_popdata_requires_ring_registry_and_positive_size() {
  test_support::RuntimeNamesGuard guard;
  Ring::names.clear();
  Ring::count = 0;
  CHECK_THROWS(std::out_of_range, PopData(5, {0.2, 0.2, 0.2, 0.2, 0.2}), "Invalid Ring value");
  test_support::install_names({"base"});
  CHECK_THROWS(std::invalid_argument, PopData(0, {0.2, 0.2, 0.2, 0.2, 0.2}), "");
}

void test_agentview_rejects_index_zero_and_past_end() {
  test_support::RuntimeNamesGuard guard;
  test_support::install_names({"base"});
  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  CHECK_THROWS(std::runtime_error, (void)pop.agent(0), "");
  CHECK_THROWS(std::runtime_error, (void)pop.agent(6), "");
  CHECK_EQ(pop.agent(5).id, size_t{5});
  pop.agent(3).status() = DEAD;  // AgentView writes through to PopData
  CHECK_EQ(pop.status[3], DEAD);
}

// register_trait_name is the single gate the loaders use for runtime names.
void test_register_trait_name_rules() {
  vector<string> names;
  CHECK_THROWS(std::logic_error, register_trait_name(names, "a"), "zero entry");
  names = {"none"};
  register_trait_name(names, "Alpha");
  CHECK_EQ(names.size(), size_t{2});
  CHECK_THROWS(std::invalid_argument, register_trait_name(names, ""), "empty");
  CHECK_THROWS(std::invalid_argument, register_trait_name(names, "Alpha"), "Duplicate");
  CHECK_THROWS(std::invalid_argument, register_trait_name(names, "ALPHA"), "Duplicate");
  CHECK_THROWS(std::invalid_argument, register_trait_name(names, "None"), "reserved");
  names = {"none"};
  for (int i = 1; i < 255; ++i) register_trait_name(names, fmt::format("n{}", i));
  CHECK_EQ(names.size(), size_t{255});
  CHECK_THROWS(std::out_of_range, register_trait_name(names, "one_too_many"), "Too many");
}

// trait id/name lookups used by history selection
void test_trait_value_lookups_and_bounds() {
  test_support::RuntimeNamesGuard guard;
  test_support::install_names({"base", "delta"}, {"pfizer"}, {"jail", "school"});
  CHECK_EQ(trait_lookup(TraitID::variant), string("variant"));
  CHECK_EQ(trait_lookup(string("status")), uint8_t(TraitID::status));
  CHECK_EQ(trait_lookup(string("not_a_trait")), uint8_t{0});

  CHECK_EQ(tr_val_to_id(TraitID::status, "dead"), uint8_t(DEAD));
  CHECK_EQ(tr_val_to_id(TraitID::variant, "delta"), uint8_t{2});
  CHECK_EQ(tr_val_to_id(TraitID::ring, "school"), uint8_t{2});
  CHECK_EQ(tr_val_to_id(TraitID::agegrp, "age60_79"), uint8_t(AGE60_79));
  CHECK_EQ(tr_val_to_string(TraitID::vax, 1), std::string_view("pfizer"));
  CHECK_THROWS(std::invalid_argument, (void)tr_val_to_id(TraitID::variant, "omega"), "unknown name");
  CHECK_THROWS(std::out_of_range, (void)tr_val_to_id(TraitID::duration, "x"), "no members");
  CHECK_THROWS(std::out_of_range, (void)tr_val_to_id(uint8_t{0}, "x"), "out of range");
  CHECK_THROWS(std::out_of_range, (void)tr_val_to_string(TraitID::vax, 2), "out of range");

  // trait_table holds references, so it sees names installed at runtime
  CHECK(&trait_table[std::to_underlying(TraitID::variant)].names == &Variant::names);
  CHECK_EQ(trait_table[std::to_underlying(TraitID::ring)].count, uint16_t{2});
}

// Loaders must leave names and counts consistent (Histories sizes use count).
void test_loaders_keep_runtime_names_and_counts_consistent() {
  test_support::RuntimeNamesGuard guard;
  const auto paths = test_support::sample_paths();
  auto [ip, ps, vnames] = load_infect_params(paths.variants);
  CHECK_EQ(Variant::names, vnames);
  CHECK_EQ(size_t{Variant::count}, Variant::names.size() - 1);

  VaxSet vs = load_vax_data(paths.vaccines, true);
  CHECK_EQ(Vax::names, vs.names);
  CHECK_EQ(size_t{Vax::count}, Vax::names.size() - 1);
  VaxSet off = load_vax_data(paths.vaccines, false);
  CHECK_MSG(Vax::names.empty() && Vax::count == 0 && off.size() == 0,
            "dovax=false leaves names {} count {}", Vax::names, Vax::count);

  (void)load_ring_traits(paths.rings, false);
  CHECK_EQ(Ring::names, (vector<string>{"error", "population"}));
  CHECK_EQ(Ring::count, uint16_t{1});
  (void)load_ring_traits(paths.rings, true);
  CHECK_EQ(Ring::names, (vector<string>{"error", "ring_1", "ring_2"}));
  CHECK_EQ(Ring::count, uint16_t{2});

  (void)load_sd_cases(json::parse(social_dist_json), load_social_params(paths.social));
  CHECK_EQ(SDCase::names, (vector<string>{"none", "young", "old"}));
}

}  // namespace

void run_traits_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  run_test("primitive_wrappers", test_primitive_wrappers);
  run_test("wrapper_comparisons", test_wrapper_comparisons);
  run_test("compile_time_trait_names_and_lookup", test_compile_time_trait_names_and_lookup);
  run_test("fixed_metadata_round_trips", test_fixed_metadata_round_trips);
  run_test("agegrp_string_constructor", test_agegrp_string_constructor_round_trips_all_names);
  run_test("status_and_condition_string_constructors", test_status_and_condition_string_constructors);
  run_test("runtime_traits_resolve_existing_names", test_runtime_traits_resolve_existing_names);
  run_test("ring_resolves_existing_names", test_ring_resolves_existing_names);
  run_test("runtime_histories_render_values", test_runtime_histories_render_values);
  run_test("runtime_trait_lookup", test_runtime_trait_lookup);
  run_test("histories_empty_state", test_histories_empty_state);
  run_test("runtime_histories_overflow", test_runtime_histories_overflow);
  run_test("scalar_histories_overflow", test_scalar_histories_overflow);
  run_test("popdata_initial_state", test_popdata_initial_state);
  run_test("popdata_requires_ring_registry", test_popdata_requires_ring_registry_and_positive_size);
  run_test("agentview_bounds", test_agentview_rejects_index_zero_and_past_end);
  run_test("register_trait_name_rules", test_register_trait_name_rules);
  run_test("trait_value_lookups_and_bounds", test_trait_value_lookups_and_bounds);
  run_test("loaders_keep_names_and_counts", test_loaders_keep_runtime_names_and_counts_consistent);
  run_test("artifact", [&] { write_traits_artifact(options); });
}
