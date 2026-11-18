#include "vaccination.h"

// vax.cpp

#include "parameters.h"
#include "population.h"
#include "random.h"
#include "series.h"
#include "sim.h"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------
// internal helpers
// ---------------------------------------------------------------

static const Vaxparam& vax_params(const VaxSet& vaxset, Vax vax) {
    return vaxset.at(vax);
}

// find index of a vax within sched.vaxesincluded
static size_t spec_index(const vector<PerVaxSpec>& specs, Vax vax) {
    for (size_t i = 0; i < specs.size(); ++i)
        if (specs[i].vax == vax) return i;
    throw std::runtime_error(fmt::format("spec_index: vax not in schedule: {}", vax.show()));
}

static void record_vaccination(AgentView agent, Vax vax, int today) {
    auto& vax_history = agent.vax_hist();
    auto& vaxday_history = agent.vaxday_hist();
    const bool history_overflow = vax_history.count >= 16 || vaxday_history.count >= 16;

    agent.vax() = vax;
    agent.vaxday() = today;
    vax_history.set(vax);
    vaxday_history.set(static_cast<int16_t>(today));

    if (history_overflow && sim::debug) {
        std::cerr << "Vaccine history overflow for person " << agent.id
                  << ". Oldest history entries lost.\n";
        std::cerr << "vax_hist.count increased to " << static_cast<int>(vax_history.count)
                  << ", vaxday_hist.count increased to "
                  << static_cast<int>(vaxday_history.count) << "\n";
    }
}

// ---------------------------------------------------------------
// doshots
// ---------------------------------------------------------------

static void doshots(
        int today,
        VaxSched& sched,
        const VaxSet& vaxset,
        vector<int>& doses_today,
        const absl::flat_hash_map<uint8_t, int>& delay2ndshot,
        const absl::flat_hash_map<uint8_t, int>& delaybooster,
        vector<size_t>& eligible,
        PopData& pop,
        Histories& histories)
{
    auto& specs = sched.vaxesincluded;

    int avail_doses = 0;
    for (int d : doses_today) avail_doses += d;
    if (avail_doses <= 0) return;

    // mix weights for categorical brand draw
    vector<float> mix;
    mix.reserve(specs.size());
    for (const auto& s : specs) mix.push_back(s.mix);

    for (size_t p : eligible) {

        if (avail_doses <= 0) break;

        auto agent = pop.agent(p);
        Vaxstatus vstatus = agent.vaxstatus();

        // ---- first shot ----
        if (vstatus == Vaxstat::none) {

            size_t choice_idx = xo::categorical_fast(mix);
            Vax choice        = specs[choice_idx].vax;

            // try alternates if out of first choice
            if (doses_today[choice_idx] < 1) {
                choice = Vax{};
                for (const Vax alt : specs[choice_idx].alternate) {
                    for (size_t ai = 0; ai < specs.size(); ++ai) {
                        if (specs[ai].vax == alt && doses_today[ai] > 0) {
                            choice     = alt;
                            choice_idx = ai;
                            break;
                        }
                    }
                    if (idx(choice) != 0) break;
                }
                if (idx(choice) == 0) continue;  // no doses of any brand
            }

            --specs[choice_idx].doses;      
            --doses_today[choice_idx];
            --avail_doses;

            record_vaccination(agent, choice, today);

            agent.vaxstatus() = (vax_params(vaxset, choice).reqdshots > 1)
                                 ? Vaxstat::first
                                 : Vaxstat::full;

            /*
            a bit tricky here.  for history tracked during simulation, we only 
            record the first shot to indicate a person has been vaccinated. we don't 
            track how many shots, boosters, etc regardless of a specific vaccine's dosage.
            The PopData columns track a history of up to 16 vaccination "events" per
            person.  This can be queried at the completion of the simulation for a 
            detailed view of which shots were dispensed of which vaccine.
            */                     
            sim::history_timing.start();
            histories.update(TraitID::vax, Phase::new_, uint8_t(choice), agent.ring().v, agent.agegrp(), today, 1);
            histories.update(TraitID::vax, Phase::now, uint8_t(choice), agent.ring().v, agent.agegrp(), today, 1);
            sim::history_timing.cum();

        // ---- second shot ----
        } else if (vstatus == Vaxstat::first) {

            const Vax last_vax = agent.vax();
            size_t  sidx      = spec_index(specs, last_vax);

            if (doses_today[sidx] < 1) continue;

            const int16_t prev_day = agent.vaxday();
            if ((today - prev_day) < delay2ndshot.at(idx(last_vax))) continue;

            if (!xo::bernoulli(specs[sidx].pct2ndshot)) continue;

            --specs[sidx].doses;  
            --doses_today[sidx];
            --avail_doses;

            record_vaccination(agent, last_vax, today);
            agent.vaxstatus() = Vaxstat::full;

        // ---- booster ----
        } else if (vstatus == Vaxstat::full || vstatus == Vaxstat::booster) {

            const Vax last_vax = agent.vax();
            size_t  sidx      = spec_index(specs, last_vax);

            if (doses_today[sidx] < 1) continue;

            const int16_t prev_day = agent.vaxday();
            if ((today - prev_day) < delaybooster.at(idx(last_vax))) continue;

            if (!xo::bernoulli(specs[sidx].pctboost)) continue;

            --specs[sidx].doses;   
            --doses_today[sidx];
            --avail_doses;
            // replace with a give_shot that preserves invariants for the vaccination columns
            // add a day stat for newly vaccinated.
            record_vaccination(agent, last_vax, today);
            agent.vaxstatus() = Vaxstat::booster;
        }
    }
}

namespace {

// Return the fractions of starting stock released through yesterday and today.
std::pair<double, double> cumulative_dose_fractions(const VaxSched& sched, int today) {
    const auto& pattern = sched.pattern;
    const auto [first_day, last_day] = sched.dayrange;
    if (pattern.size() < 2 || first_day > last_day) return {};

    const double days = static_cast<double>(last_day) - first_day + 1.0;
    const double segments = static_cast<double>(pattern.size() - 1);
    const auto area_through = [&](double elapsed_days) {
        const double position = std::clamp(elapsed_days / days, 0.0, 1.0) * segments;
        double area = 0.0;
        for (size_t i = 0; i + 1 < pattern.size(); ++i) {
            const double part = std::min(1.0, position - static_cast<double>(i));
            if (part <= 0.0) break;

            // Integral of the two linear endpoint weights over this segment.
            const double tail = 1.0 - part;
            const double left = pattern[i] * (1.0 - tail * tail);
            const double right = pattern[i + 1] * (part * part);
            area += (left + right) / 2.0;
        }
        return area;
    };

    const double total_area = area_through(days);
    // Correct numeric drift around unit area while preserving larger input errors.
    const double divisor = std::abs(total_area - 1.0) <= 1e-6 ? total_area : 1.0;
    const double elapsed = static_cast<double>(today) - first_day;
    return {
        (area_through(elapsed) / divisor) * sched.targetpct,
        (area_through(elapsed + 1.0) / divisor) * sched.targetpct,
    };
}

}  // namespace

static void vaccinate_sched(int today,
                            VaxSched& sched,
                            const VaxSet& vaxset,
                            PopData& pop,
                            Histories& histories)
{
    auto& specs    = sched.vaxesincluded;
    auto& dayrange = sched.dayrange;

    // Keep the follow-up window. The schedule releases doses within dayrange.
    int maxdelay = 0;
    for (const auto& spec : specs)
        maxdelay = std::max(maxdelay, vax_params(vaxset, spec.vax).delay2ndshot);

    if (today < dayrange.first || today > dayrange.second + maxdelay) return;

    // pre-build delay maps — one lookup per vax brand, not per person
    absl::flat_hash_map<uint8_t, int> delay2ndshot, delaybooster;
    for (const auto& spec : specs) {
        const auto& vp        = vax_params(vaxset, spec.vax);
        delay2ndshot[idx(spec.vax)] = vp.delay2ndshot;
        delaybooster[idx(spec.vax)] = vp.delaybooster;
    }

    // Round the cumulative amounts so fractions of a dose carry across day boundaries.
    const auto [fraction_before, fraction_through] = cumulative_dose_fractions(sched, today);
    vector<int> doses_today(specs.size());
    for (size_t i = 0; i < specs.size(); ++i) {
        const auto& spec = specs[i];
        const double planned = std::floor(spec.starting_doses * fraction_through)
                             - std::floor(spec.starting_doses * fraction_before);
        const double remaining = std::max(0, spec.doses);
        doses_today[i] = static_cast<int>(std::min(remaining, std::max(0.0, planned)));
    }

    // eligible: correct agegrp AND (unexposed OR recovered >= 14 days ago)
    vector<size_t> eligible;
    eligible.reserve(pop.popn / 4);

    for (size_t p = 1; p <= pop.popn; ++p) {

        bool in_filter = false;
        for (const Agegrp& fg : sched.filtervec)
            if (pop.agegrp[p] == fg) { in_filter = true; break; }
        if (!in_filter) continue;

        bool unexposed = (pop.status[p] == UNEXPOSED);
        bool recovered = (pop.status[p] == RECOVERED)
                      && (pop.recovday[p] > 0)
                      && (pop.recovday[p] < today - 14);

        if (unexposed || recovered)
            eligible.push_back(p);
    }

    std::shuffle(eligible.begin(), eligible.end(), xo::get_gen());

    doshots(today, sched, vaxset,
            doses_today, delay2ndshot, delaybooster,
            eligible, pop, histories);
}

// ---------------------------------------------------------------
// vaccinate  (called once per simulation day)
// ---------------------------------------------------------------

void vaccinate(int today,
               VaxSchedSet& schedset,
               const VaxSet& vaxset,
               PopData& pop,
               Histories& histories)
{
    for (auto& [name, sched] : schedset.schedules) {
        (void)name;
        vaccinate_sched(today, sched, vaxset, pop, histories);
    }
}
