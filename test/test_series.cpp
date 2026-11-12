// series: the Histories container -- vector layout and order, metadata, the two lookup
// maps, seeding, carry-forward, totals, selection, and CSV output.
#include "test_support.h"

namespace {

constexpr std::string_view GROUP = "series";
using test_support::install_names;
using test_support::make_pop;
using test_support::RuntimeNamesGuard;

void install_counts(size_t variants, size_t vaxes, size_t rings) {
  vector<string> v, x, r;
  for (size_t i = 1; i <= variants; ++i) v.push_back(fmt::format("variant_{}", i));
  for (size_t i = 1; i <= vaxes; ++i) x.push_back(fmt::format("vax_{}", i));
  for (size_t i = 1; i <= rings; ++i) r.push_back(fmt::format("ring_{}", i));
  install_names(v, x, r);
}

const vector<string>& value_names(TraitID t) {
  return t == TraitID::status ? Status::names : t == TraitID::vax ? Vax::names : Variant::names;
}

size_t values_per_trait_total() { return Status::count + Vax::count + Variant::count; }

// vector counts follow from the registries: (status+vax+variant values) x 2 phases x rings x ages
void test_layout_counts_across_cardinalities() {
  RuntimeNamesGuard guard;
  for (size_t nv = 1; nv <= 3; ++nv)
    for (size_t nx = 0; nx <= 2; ++nx)
      for (size_t nr = 1; nr <= 3; ++nr) {
        install_counts(nv, nx, nr);
        auto pop = make_pop(10, nr);
        Histories h(3, pop);
        const size_t vals = values_per_trait_total();
        const size_t sim = vals * 2 * nr * Agegrp::count;
        const size_t tot = vals * 2 * (nr == 1 ? 1 : 1 + nr + Agegrp::count);
        CHECK_MSG(h.sim_history_count == sim && h.total_history_count == tot &&
                      h.all_history_count == sim + tot && h.history_vectors_.size() == sim + tot,
                  "variants {} vaxes {} rings {}: sim {} (want {}) total {} (want {}) all {}", nv, nx, nr,
                  h.sim_history_count, sim, h.total_history_count, tot, h.history_vectors_.size());
        CHECK_EQ(h.sim_history_map.size(), sim);
        CHECK_EQ(h.cumulative_columns.size() + h.new_columns.size(), sim);
        int bad_len = 0;
        for (const auto& hv : h.history_vectors_) bad_len += hv.data.size() != 4;
        CHECK_MSG(bad_len == 0, "{} vectors not sized day_cnt + 1", bad_len);
      }
}

// sim map: bijection onto [0, sim_count), metadata agrees with the key, label format,
// and storage order is trait (status, vax, variant) > phase > value > ring > age
void test_sim_map_metadata_and_order() {
  RuntimeNamesGuard guard;
  install_counts(2, 2, 2);
  auto pop = make_pop(10, 2);
  Histories h(2, pop);
  vector<int> hits(h.sim_history_count, 0);
  int bad = 0;
  std::string first;
  for (const auto& [k, idx] : h.sim_history_map) {
    REQUIRE(idx < h.sim_history_count);
    ++hits[idx];
    const auto& m = h.history_vectors_[idx].meta;
    const auto t = static_cast<TraitID>(k.trait);
    const string label = fmt::format("{}|{}|{}|{}|{}", trait_lookup(t), magic_enum::enum_name(static_cast<Phase>(k.phase)),
                                     value_names(t)[k.trval], Ring::names[k.ring], Agegrp::names[k.age]);
    const bool ok = std::to_underlying(m.traitnum) == k.trait && std::to_underlying(m.phasenum) == k.phase &&
                    m.traitval_num == k.trval && m.ringnum.v == k.ring && m.agenum.v == k.age &&
                    m.label == label && h.label(idx) == label;
    if (!ok && bad++ == 0) first = fmt::format("{} -> {} has label '{}'", k, idx, m.label);
  }
  CHECK_MSG(bad == 0, "{} sim-map entries disagree with metadata; first: {}", bad, first);
  CHECK_MSG(std::ranges::all_of(hits, [](int x) { return x == 1; }), "sim map is not a bijection");

  auto rank = [](TraitID t) { return t == TraitID::status ? 0 : t == TraitID::vax ? 1 : 2; };
  auto key = [&](size_t i) {
    const auto& m = h.history_vectors_[i].meta;
    return std::tuple{rank(m.traitnum), std::to_underlying(m.phasenum), m.traitval_num, m.ringnum.v, m.agenum.v};
  };
  int order_bad = 0;
  for (size_t i = 1; i < h.sim_history_count; ++i) order_bad += !(key(i - 1) < key(i));
  CHECK_MSG(order_bad == 0, "{} adjacent simulation vectors out of order", order_bad);
  CHECK_EQ(h.history_vector_idx(TraitID::status, Phase::now, UNEXPOSED.v, AGE0_19, 1), size_t{0});
  CHECK_EQ(h.history_vector_idx(TraitID::variant, Phase::new_, 2, AGE80_UP, 2), h.sim_history_count - 1);
}

// totals map with several rings: every key points at a total vector whose metadata agrees
void test_totals_map_multi_ring() {
  RuntimeNamesGuard guard;
  install_counts(1, 1, 3);
  auto pop = make_pop(12, 3);
  Histories h(2, pop);
  int bad = 0;
  std::string first;
  for (const auto& [k, idx] : h.sim_totals_map) {
    const auto& m = h.history_vectors_[idx].meta;
    const bool ok = idx >= h.sim_history_count && std::to_underlying(m.traitnum) == k.trait &&
                    std::to_underlying(m.phasenum) == k.phase && m.traitval_num == k.trval &&
                    m.ringnum.v == k.ring && m.agenum.v == k.age;
    if (!ok && bad++ == 0) first = fmt::format("{} -> {} ({})", k, idx, m.label);
  }
  CHECK_MSG(bad == 0, "{} totals-map entries disagree with metadata; first: {}", bad, first);
  CHECK_EQ(h.sim_totals_map.size(), h.total_history_count);
}

// with a single ring the split totals are aliases: (all rings, age a) is the ring-1 cell
// and (ring 1, all ages) is the full total; no extra vectors are created
void test_totals_map_single_ring_aliases() {
  RuntimeNamesGuard guard;
  install_counts(1, 0, 1);
  auto pop = make_pop(10, 1);
  Histories h(2, pop);
  const uint8_t v = INFECTIOUS.v;
  const size_t full = h.history_vector_idx(TraitID::status, Phase::now, v, Agegrp{0}, RING_ALL, true);
  CHECK_EQ(h.history_vector_idx(TraitID::status, Phase::now, v, Agegrp{0}, 1, true), full);
  for (uint8_t a = 1; a <= Agegrp::count; ++a)
    CHECK_EQ(h.history_vector_idx(TraitID::status, Phase::now, v, Agegrp{a}, RING_ALL, true),
             h.history_vector_idx(TraitID::status, Phase::now, v, Agegrp{a}, 1));
}

// constructor seeds day-1 now_unexposed per (ring, age) from PopData; nothing else
void test_constructor_seeds_unexposed_by_ring_and_age() {
  RuntimeNamesGuard guard;
  install_counts(1, 0, 2);
  auto pop = make_pop(23, 2);
  Histories h(4, pop);
  int bad = 0;
  for (uint8_t r = 1; r <= 2; ++r)
    for (uint8_t a = 1; a <= Agegrp::count; ++a) {
      int want = 0;
      for (size_t p = 1; p <= pop.popn; ++p) want += pop.ring[p].v == r && pop.agegrp[p].v == a;
      bad += h.at(TraitID::status, Phase::now, UNEXPOSED.v, Agegrp{a}, r)[1] != want;
    }
  CHECK_MSG(bad == 0, "{} (ring, age) cells seeded wrongly", bad);
  long other = 0;
  for (size_t i = 0; i < h.sim_history_count; ++i)
    for (size_t d = 0; d <= 4; ++d)
      if (!(h.history_vectors_[i].meta.traitnum == TraitID::status && h.history_vectors_[i].meta.phasenum == Phase::now &&
            h.history_vectors_[i].meta.traitval_num == UNEXPOSED.v && d == 1))
        other += h.history_vectors_[i].data[d];
  CHECK_EQ(other, 0L);

  pop.ring[3] = Ring{};  // ring 0 is not a valid lane
  CHECK_THROWS(std::runtime_error, (Histories(4, pop)), "Cannot seed history for person 3");
}

void test_newday_carries_now_but_not_new() {
  RuntimeNamesGuard guard;
  install_counts(1, 1, 1);
  auto pop = make_pop(5, 1);
  Histories h(3, pop);
  h.update(TraitID::status, Phase::now, INFECTIOUS.v, 1, AGE20_39, 1, 2);
  h.update(TraitID::status, Phase::new_, INFECTIOUS.v, 1, AGE20_39, 1, 2);
  h.update(TraitID::vax, Phase::now, 1, 1, AGE20_39, 1, 3);
  h.init_history_newday(2);
  CHECK_EQ(h.at(TraitID::status, Phase::now, INFECTIOUS.v, AGE20_39, 1)[2], 2);
  CHECK_EQ(h.at(TraitID::status, Phase::now, UNEXPOSED.v, AGE20_39, 1)[2], 1);
  CHECK_EQ(h.at(TraitID::vax, Phase::now, 1, AGE20_39, 1)[2], 3);
  CHECK_EQ(h.at(TraitID::status, Phase::new_, INFECTIOUS.v, AGE20_39, 1)[2], 0);
  h.init_history_newday(1);  // day 1 is the seed day: unchanged
  CHECK_EQ(h.at(TraitID::status, Phase::now, UNEXPOSED.v, AGE20_39, 1)[1], 1);
}

// sum_total_histories against an independent brute-force sum, for 1 and 3 rings
void test_sum_total_histories_matches_brute_force() {
  RuntimeNamesGuard guard;
  for (size_t nr : {size_t{1}, size_t{3}}) {
    install_counts(2, 2, nr);
    auto pop = make_pop(30, nr);
    Histories h(5, pop);
    for (size_t i = 0; i < h.sim_history_count; ++i)
      for (size_t d = 1; d <= 5; ++d) h.history_vectors_[i].data[d] = static_cast<HistoryValue>((i * 7 + d * 3) % 11);
    h.sum_total_histories();
    int bad = 0;
    std::string first;
    for (const auto& [k, idx] : h.sim_totals_map) {
      for (size_t d = 1; d <= 5; ++d) {
        HistoryValue want = 0;
        for (uint8_t r = 1; r <= nr; ++r)
          for (uint8_t a = 1; a <= Agegrp::count; ++a)
            if ((k.ring == RING_ALL || k.ring == r) && (k.age == 0 || k.age == a))
              want += h.at(static_cast<TraitID>(k.trait), static_cast<Phase>(k.phase), k.trval, Agegrp{a}, r)[d];
        if (h.history_vectors_[idx].data[d] != want && bad++ == 0)
          first = fmt::format("{} day {}: {} want {}", k, d, h.history_vectors_[idx].data[d], want);
      }
    }
    CHECK_MSG(bad == 0, "rings {}: {} total cells wrong; first: {}", nr, bad, first);
    for (size_t d = 1; d <= 5; ++d)
      CHECK_EQ(h.aggregate_value(TraitID::vax, Phase::new_, 2, d),
               test_support::total(h, TraitID::vax, Phase::new_, 2, d));
  }
}

// select_indices with 5-field selectors {trait, phase, value, age, ring}
void test_select_indices() {
  RuntimeNamesGuard guard;
  install_counts(1, 1, 2);
  auto pop = make_pop(10, 2);
  Histories h(2, pop);
  auto one = [&](HistorySelector s) {
    const auto v = h.select_indices(HistorySelectorSet{s});
    return v.size() == 1 ? v[0] : size_t(-1);
  };
  CHECK_EQ(one({"status", "now", "dead", "age60_79", "ring_2"}),
           h.history_vector_idx(TraitID::status, Phase::now, DEAD.v, AGE60_79, 2));
  CHECK_EQ(one({"vax", "new_", "vax_1", "total", "total"}),
           h.history_vector_idx(TraitID::vax, Phase::new_, 1, Agegrp{0}, RING_ALL, true));
  CHECK_EQ(one({"variant", "now", "variant_1", "total", "ring_1"}),
           h.history_vector_idx(TraitID::variant, Phase::now, 1, Agegrp{0}, 1, true));
  CHECK_EQ(one({"status", "now", "recovered", "age0_19", "total"}),
           h.history_vector_idx(TraitID::status, Phase::now, RECOVERED.v, AGE0_19, RING_ALL, true));

  const auto all = h.select_indices({}, "all");
  CHECK_EQ(all.size(), h.all_history_count);
  const auto tot = h.select_indices({}, "total");
  REQUIRE(tot.size() == h.total_history_count);
  CHECK_EQ(tot.front(), h.sim_history_count);

  // order of the selection is kept; a key that does not exist is skipped
  const auto two = h.select_indices(HistorySelectorSet{{"status", "now", "none", "total", "total"},
                                                       {"status", "now", "dead", "total", "total"}});
  CHECK_EQ(two.size(), size_t{1});
  // unknown names are an error
  CHECK_THROWS(std::invalid_argument,
               (void)h.select_indices(HistorySelectorSet{{"status", "now", "deceased", "total", "total"}}), "");
  CHECK_THROWS(std::invalid_argument,
               (void)h.select_indices(HistorySelectorSet{{"status", "now", "dead", "age0_19", "ring_9"}}), "");
}

// CSV output: header = labels in selection order, one row per day, values from the vectors
void test_serialize_histories_round_trip() {
  RuntimeNamesGuard guard;
  install_counts(1, 0, 2);
  auto pop = make_pop(10, 2);
  Histories h(3, pop);
  h.at(TraitID::status, Phase::now, INFECTIOUS.v, AGE0_19, 1)[2] = 4;
  h.at(TraitID::status, Phase::now, INFECTIOUS.v, AGE20_39, 2)[3] = 5;
  h.sum_total_histories();
  const auto path = test_support::fs::temp_directory_path() / test_support::unique_name("epi_series_") / "h.csv";
  const HistorySelectorSet sel{{"status", "now", "infectious", "total", "total"},
                               {"status", "now", "infectious", "age0_19", "ring_1"}};
  serialize_histories(h, sel, "", path);
  const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(path));
  REQUIRE(lines.size() == 4);
  CHECK_EQ(lines[0], string("status|now|infectious|total|total,status|now|infectious|ring_1|age0_19"));
  CHECK_EQ(lines[1], string("0,0"));
  CHECK_EQ(lines[2], string("4,4"));
  CHECK_EQ(lines[3], string("5,0"));

  serialize_histories(h, {}, "all", path);
  const auto all = test_support::split_trimmed_lines(test_support::read_file_text(path));
  REQUIRE(all.size() == 4);
  CHECK_EQ(static_cast<size_t>(std::ranges::count(all[0], ',')) + 1, h.all_history_count);
  test_support::fs::remove_all(path.parent_path());
}

void test_print_and_enumerate() {
  RuntimeNamesGuard guard;
  install_counts(1, 0, 1);
  auto pop = make_pop(10, 1);
  Histories h(2, pop);
  h.sum_total_histories();
  std::ostringstream out;
  print_total_status_histories(h, 15, out);
  CHECK(out.str().contains("status|now|unexposed|total|total"));
  CHECK(out.str().contains("      10"));
  const auto sel = enumerate_history_selections(h);
  REQUIRE(sel.size() == h.all_history_count);
  CHECK_EQ(sel[0].trait, string("status"));
  CHECK_EQ(sel[0].ring, Ring::names[1]);
  CHECK_EQ(sel.back().age, string("total"));
}

void write_series_artifacts(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;
  RuntimeNamesGuard guard;
  install_counts(2, 2, 2);
  auto pop = make_pop(10, 2);
  Histories h(2, pop);
  std::ostringstream summary;
  summary << fmt::format("sim vectors {} total vectors {}\n\n", h.sim_history_count, h.total_history_count);
  h.dump_history_layout(summary);
  test_support::write_artifact_text(options, GROUP, "history_layout.txt", summary.str());
}

}  // namespace

void run_series_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  run_test("layout_counts_across_cardinalities", test_layout_counts_across_cardinalities);
  run_test("sim_map_metadata_and_order", test_sim_map_metadata_and_order);
  run_test("totals_map_multi_ring", test_totals_map_multi_ring);
  run_test("totals_map_single_ring_aliases", test_totals_map_single_ring_aliases);
  run_test("constructor_seeds_unexposed", test_constructor_seeds_unexposed_by_ring_and_age);
  run_test("newday_carries_now_not_new", test_newday_carries_now_but_not_new);
  run_test("sum_total_histories_brute_force", test_sum_total_histories_matches_brute_force);
  run_test("select_indices", test_select_indices);
  run_test("serialize_histories_round_trip", test_serialize_histories_round_trip);
  run_test("print_and_enumerate", test_print_and_enumerate);
  run_test("artifact", [&] { write_series_artifacts(options); });
}
