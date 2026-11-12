// test_invariants.h -- whole-run consistency checks over PopData + Histories.
//
// Run after any simulation (runsim already calls sum_total_histories). Each invariant is
// one aggregated check: on failure it prints the number of violations, the first example,
// and a hint pointing at the unit tests that isolate the responsible code.
#pragma once

#include "test_support.h"

namespace invariants {

struct Report {
  int violations{0};
  std::string first;
  void add(std::string msg) {
    if (violations++ == 0) first = std::move(msg);
  }
};

inline bool report(std::string_view label, std::string_view name, const Report& r,
                   std::string_view hint) {
  return CHECK_MSG(r.violations == 0, "[{}] invariant '{}': {} violation(s); first: {}\n       hint: {}",
                   label, name, r.violations, r.first, hint);
}

// sum of an atomic trait/phase/value over all rings and ages
inline HistoryValue all_cells(const Histories& h, TraitID t, Phase ph, uint8_t v, size_t d) {
  HistoryValue s = 0;
  for (uint8_t r = 1; r <= Ring::count; ++r)
    for (uint8_t a = 1; a <= Agegrp::count; ++a) s += h.at(t, ph, v, Agegrp{a}, r)[d];
  return s;
}

// Returns the number of failed invariants (0 = all hold).
inline int check_run(const PopData& pop, const Histories& h, std::string_view label) {
  const size_t nd = h.day_cnt;
  const auto S = TraitID::status, VAR = TraitID::variant, VX = TraitID::vax;
  const auto NOW = Phase::now, NEW = Phase::new_;
  const uint8_t UNEXP = UNEXPOSED, INF = INFECTIOUS, REC = RECOVERED, DED = DEAD;
  int failed = 0;
  auto chk = [&](std::string_view name, const Report& r, std::string_view hint) {
    if (!report(label, name, r, hint)) ++failed;
  };

  // 1. population conservation each day
  {
    Report r;
    for (size_t d = 1; d <= nd; ++d) {
      HistoryValue s = 0;
      for (uint8_t v = 1; v <= Status::count; ++v) s += all_cells(h, S, NOW, v, d);
      if (s != static_cast<HistoryValue>(pop.popn))
        r.add(fmt::format("day {}: sum(now status) = {} != popn {}", d, s, pop.popn));
    }
    chk("status conservation", r,
        "make_sick/make_well/make_dead +/-1 pairs (disease_modeling: state_transitions_*), "
        "Histories ctor seeding and init_history_newday (series)");
  }
  // 2. variant(now) total equals infectious(now)
  {
    Report r;
    for (size_t d = 1; d <= nd; ++d) {
      HistoryValue sv = 0;
      for (uint8_t v = 1; v <= Variant::count; ++v) sv += all_cells(h, VAR, NOW, v, d);
      const auto inf = all_cells(h, S, NOW, INF, d);
      if (sv != inf) r.add(fmt::format("day {}: sum(now variant) = {} != now infectious {}", d, sv, inf));
    }
    chk("variant matches infectious", r,
        "variant history updates in make_sick (+1) and make_well/make_dead (-1) (disease_modeling)");
  }
  // 3-4. flow balance: now changes equal the day's new events
  {
    Report rd, ri, rn;
    for (size_t d = 1; d <= nd; ++d) {
      auto prev = [&](uint8_t v) { return d == 1 ? HistoryValue{0} : all_cells(h, S, NOW, v, d - 1); };
      const auto nd_ = all_cells(h, S, NEW, DED, d), ni = all_cells(h, S, NEW, INF, d),
                 nr = all_cells(h, S, NEW, REC, d);
      if (all_cells(h, S, NOW, DED, d) - prev(DED) != nd_)
        rd.add(fmt::format("day {}: delta now_dead != new_dead {}", d, nd_));
      if (d > 1 && all_cells(h, S, NOW, INF, d) - prev(INF) != ni - nr - nd_)
        ri.add(fmt::format("day {}: delta now_infectious {} != new_inf - new_recov - new_dead = {}", d,
                           all_cells(h, S, NOW, INF, d) - prev(INF), ni - nr - nd_));
      for (uint8_t v = 1; v <= Status::count; ++v) {
        if (all_cells(h, S, NEW, v, d) < 0 || all_cells(h, S, NOW, v, d) < 0)
          rn.add(fmt::format("day {}: negative count for status {}", d, Status::names[v]));
      }
      if (all_cells(h, S, NEW, UNEXP, d) != 0) rn.add(fmt::format("day {}: new_unexposed written", d));
    }
    chk("now_dead flow", rd, "make_dead history updates (disease_modeling)");
    chk("now_infectious flow", ri, "make_sick/make_well/make_dead now+new pairs (disease_modeling)");
    chk("non-negative counts", rn, "unguarded transitions, e.g. make_well on a non-infectious person");
  }
  // 5. history event totals equal per-person event counts
  {
    long sum_new_inf = 0, sum_new_rec = 0, sum_new_dead = 0;
    for (size_t d = 1; d <= nd; ++d) {
      sum_new_inf += all_cells(h, S, NEW, INF, d);
      sum_new_rec += all_cells(h, S, NEW, REC, d);
      sum_new_dead += all_cells(h, S, NEW, DED, d);
    }
    long infections = 0, recoveries = 0, dead = 0;
    for (size_t p = 1; p <= pop.popn; ++p) {
      infections += pop.variant_hist[p].count;
      recoveries += pop.recovday_hist[p].count;
      dead += pop.status[p] == DEAD;
    }
    Report r;
    if (sum_new_inf != infections)
      r.add(fmt::format("sum(new_infectious) {} != sum(variant_hist.count) {}", sum_new_inf, infections));
    if (sum_new_rec != recoveries)
      r.add(fmt::format("sum(new_recovered) {} != sum(recovday_hist.count) {}", sum_new_rec, recoveries));
    if (sum_new_dead != dead) r.add(fmt::format("sum(new_dead) {} != dead people {}", sum_new_dead, dead));
    chk("events match PopData", r, "history writes vs PopData writes in make_sick/make_well/make_dead");
  }
  // 6. final-day histories equal PopData by (status|variant, age, ring)
  {
    Report r;
    std::vector<int> cnt((Status::count + 1) * (Agegrp::count + 1) * (Ring::count + 1), 0);
    std::vector<int> vcnt((Variant::count + 1) * (Agegrp::count + 1) * (Ring::count + 1), 0);
    auto key = [](size_t v, size_t a, size_t ring) {
      return (v * (Agegrp::count + 1) + a) * (Ring::count + 1) + ring;
    };
    for (size_t p = 1; p <= pop.popn; ++p) {
      const size_t a = pop.agegrp[p].v, ring = pop.ring[p].v;
      if (a < 1 || a > Agegrp::count || ring < 1 || ring > Ring::count) {
        r.add(fmt::format("person {} has agegrp {} ring {}", p, a, ring));
        continue;
      }
      ++cnt[key(pop.status[p].v, a, ring)];
      if (pop.status[p] == INFECTIOUS) ++vcnt[key(pop.variant[p].v, a, ring)];
    }
    for (uint8_t ring = 1; ring <= Ring::count; ++ring)
      for (uint8_t a = 1; a <= Agegrp::count; ++a) {
        for (uint8_t v = 1; v <= Status::count; ++v) {
          const auto hv = h.at(S, NOW, v, Agegrp{a}, ring)[nd];
          if (hv != cnt[key(v, a, ring)])
            r.add(fmt::format("status {} age {} ring {}: history {} != PopData {}", Status::names[v],
                              Agegrp::names[a], Ring::names[ring], hv, cnt[key(v, a, ring)]));
        }
        for (uint8_t v = 1; v <= Variant::count; ++v) {
          const auto hv = h.at(VAR, NOW, v, Agegrp{a}, ring)[nd];
          if (hv != vcnt[key(v, a, ring)])
            r.add(fmt::format("variant {} age {} ring {}: history {} != PopData {}", Variant::names[v],
                              Agegrp::names[a], Ring::names[ring], hv, vcnt[key(v, a, ring)]));
        }
      }
    chk("final histories match PopData", r,
        "age/ring coordinates passed to Histories::update (series: atomic_update_*) and ring assignment (setup)");
  }
  // 7. vaccination: one history count per first shot, by brand
  {
    Report r;
    std::vector<long> first_brand(Vax::count + 1, 0);
    for (size_t p = 1; p <= pop.popn; ++p) {
      const bool hist = pop.vax_hist[p].count > 0;
      if (hist != (pop.vaxstatus[p] != Vaxstat::none))
        r.add(fmt::format("person {}: vaxstatus {} but vax_hist.count {}", p, pop.vaxstatus[p].show(),
                          pop.vax_hist[p].count));
      if (hist && pop.vax_hist[p].arr[0].v <= Vax::count) ++first_brand[pop.vax_hist[p].arr[0].v];
    }
    for (uint8_t v = 1; v <= Vax::count; ++v) {
      long news = 0;
      for (size_t d = 1; d <= nd; ++d) news += all_cells(h, VX, NEW, v, d);
      const auto now = all_cells(h, VX, NOW, v, nd);
      if (news != first_brand[v] || now != first_brand[v])
        r.add(fmt::format("vax {}: sum(new) {} / final now {} != people whose first shot was it {}",
                          Vax::names[v], news, now, first_brand[v]));
    }
    chk("vaccination histories", r, "vaccinate/doshots history updates and record_vaccination (vaccination)");
  }
  // 8. totals equal brute-force sums of atomic cells (uses the vector metadata)
  {
    Report r;
    for (size_t idx = h.sim_history_count; idx < h.all_history_count; ++idx) {
      const auto& m = h.history_vectors_[idx].meta;
      for (size_t d = 1; d <= nd; ++d) {
        HistoryValue s = 0;
        for (uint8_t ring = 1; ring <= Ring::count; ++ring)
          for (uint8_t a = 1; a <= Agegrp::count; ++a)
            if ((m.ringnum.v == RING_ALL || m.ringnum.v == ring) && (m.agenum.v == 0 || m.agenum.v == a))
              s += h.at(m.traitnum, m.phasenum, m.traitval_num, Agegrp{a}, ring)[d];
        if (s != h.history_vectors_[idx].data[d]) {
          r.add(fmt::format("{} day {}: total {} != sum of cells {}", m.label, d,
                            h.history_vectors_[idx].data[d], s));
          break;
        }
      }
    }
    chk("totals equal atomic sums", r, "Histories::sum_total_histories and totals-map layout (series)");
  }
  // 9. per-person state consistency
  {
    Report r;
    for (size_t p = 1; p <= pop.popn; ++p) {
      const auto st = pop.status[p];
      const int c = pop.cond[p].v, dur = pop.duration[p].v, sick = pop.sickday[p].v;
      if (st == UNEXPOSED) {
        if (pop.variant_hist[p].count != 0 || sick != 0 || c != UNINFECTED.v || dur != 0)
          r.add(fmt::format("unexposed person {} has infection data", p));
      } else if (st == INFECTIOUS) {
        if (c < NIL.v || c > SEVERE.v || dur < 1 || dur > DURATIONLIM || pop.variant[p].v < 1 ||
            pop.variant[p].v > Variant::count || sick < 1 || pop.variant_hist[p].latest() != pop.variant[p])
          r.add(fmt::format("infectious person {}: cond {} duration {} variant {} sickday {}", p, c, dur,
                            pop.variant[p].v, sick));
      } else if (st == RECOVERED) {
        if (c != UNINFECTED.v || dur != 0 || pop.recovday[p].v < sick || sick < 1)
          r.add(fmt::format("recovered person {}: cond {} duration {} sickday {} recovday {}", p, c, dur,
                            sick, pop.recovday[p].v));
      } else if (st == DEAD) {
        if (pop.deadday[p].v < sick || sick < 1)
          r.add(fmt::format("dead person {}: sickday {} deadday {}", p, sick, pop.deadday[p].v));
      } else {
        r.add(fmt::format("person {} has status {}", p, st.v));
      }
      if (pop.sdcase[p].v >= SDCase::names.size())
        r.add(fmt::format("person {} has sdcase {} (names {})", p, pop.sdcase[p].v, SDCase::names.size()));
    }
    chk("person state consistency", r,
        "make_sick/make_well/make_dead field updates and progression duration handling (disease_modeling)");
  }
  return failed;
}

}  // namespace invariants
