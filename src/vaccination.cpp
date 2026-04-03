#include "lib_includes.h"
#include "vaccination.h"

// vax.cpp

#include "parameters.h"
#include "population.h"
#include "random.h"
#include "series.h"

// ---------------------------------------------------------------
// internal helpers
// ---------------------------------------------------------------

static const VaxParams& vax_params(const VaxSet& vaxset, const string& name) {
    for (const auto& [n, p] : vaxset.vaxset)
        if (n == name) return p;
    throw std::runtime_error("vax_params: unknown vaccine: " + name);
}

// find index of a vax name within sched.vaxesincluded
static size_t spec_index(const vector<PerVaxSpec>& specs, const string& name) {
    for (size_t i = 0; i < specs.size(); ++i)
        if (specs[i].vax_name == name) return i;
    throw std::runtime_error("spec_index: vax not in schedule: " + name);
}

// ---------------------------------------------------------------
// doshots
// ---------------------------------------------------------------

static void doshots(
        int today,
        VaxSched& sched,
        const VaxSet& vaxset,
        const MapEnum<uint8_t>& vaxlist,
        vector<int>& doses_today,
        const absl::flat_hash_map<string, int>& delay2ndshot,
        const absl::flat_hash_map<string, int>& delaybooster,
        vector<size_t>& eligible,
        PopData& pop,
        HistorySeries& series)
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
            string choice     = specs[choice_idx].vax_name;

            // try alternates if out of first choice
            if (doses_today[choice_idx] < 1) {
                choice = "";
                for (const auto& alt : specs[choice_idx].alternate) {
                    for (size_t ai = 0; ai < specs.size(); ++ai) {
                        if (specs[ai].vax_name == alt && doses_today[ai] > 0) {
                            choice     = alt;
                            choice_idx = ai;
                            break;
                        }
                    }
                    if (!choice.empty()) break;
                }
                if (choice.empty()) continue;  // no doses of any brand
            }

            --specs[choice_idx].doses;      
            --doses_today[choice_idx];
            --avail_doses;

            uint8_t vax_idx = vaxlist.lookup.at(choice);
            uint8_t vc      = agent.vax_count();
            agent.vaxrcvd()[vc] = vax_idx;
            agent.vaxday()[vc]  = static_cast<int16_t>(today);
            ++agent.vax_count();

            agent.vaxstatus() = (vax_params(vaxset, choice).reqdshots > 1)
                                 ? Vaxstat::first
                                 : Vaxstat::full;
            delta_series(series, SeriesName::new_vaccinated, agent.agegrp(), today, 1);
            delta_series(series, SeriesName::now_vaccinated, agent.agegrp(), today, 1);

        // ---- second shot ----
        } else if (vstatus == Vaxstat::first) {

            uint8_t vc        = agent.vax_count();
            uint8_t last_idx  = agent.vaxrcvd()[zidx(vc)];
            string  last_name = vaxlist.to_str(last_idx);
            size_t  sidx      = spec_index(specs, last_name);

            if (doses_today[sidx] < 1) continue;

            int16_t prev_day = agent.vaxday()[zidx(vc)];
            if ((today - prev_day) < delay2ndshot.at(last_name)) continue;

            if (!xo::bernoulli(specs[sidx].pct2ndshot)) continue;

            --specs[sidx].doses;  
            --doses_today[sidx];
            --avail_doses;

            agent.vaxrcvd()[vc] = last_idx;
            agent.vaxday()[vc]  = static_cast<int16_t>(today);
            ++agent.vax_count();
            agent.vaxstatus() = Vaxstat::full;

        // ---- booster ----
        } else if (vstatus == Vaxstat::full || vstatus == Vaxstat::booster) {

            uint8_t vc        = agent.vax_count();
            uint8_t last_idx  = agent.vaxrcvd()[zidx(vc)];
            string  last_name = vaxlist.to_str(last_idx);
            size_t  sidx      = spec_index(specs, last_name);

            if (doses_today[sidx] < 1) continue;

            int16_t prev_day = agent.vaxday()[zidx(vc)];
            if ((today - prev_day) < delaybooster.at(last_name)) continue;

            if (!xo::bernoulli(specs[sidx].pctboost)) continue;

            --specs[sidx].doses;   
            --doses_today[sidx];
            --avail_doses;
            // replace with a give_shot that preserves invariants for the vaccination columns
            // add a day stat for newly vaccinated.
            agent.vaxrcvd()[vc] = last_idx;
            agent.vaxday()[vc]  = static_cast<int16_t>(today);
            ++agent.vax_count();
            agent.vaxstatus() = Vaxstat::booster;
        }
    }
}

static void vaccinate_sched(int today,
                            VaxSched& sched,
                            const VaxSet& vaxset,
                            const MapEnum<uint8_t>& vaxlist,
                            PopData& pop,
                            HistorySeries& series)
{
    auto& specs    = sched.vaxesincluded;
    auto& dayrange = sched.dayrange;

    // extend window past schedule end to allow 2nd shots to complete
    int maxdelay = 0;
    for (const auto& spec : specs)
        maxdelay = std::max(maxdelay, vax_params(vaxset, spec.vax_name).delay2ndshot);

    if (today < dayrange.first || today > dayrange.second + maxdelay) return;

    // pre-build delay maps — one lookup per vax brand, not per person
    absl::flat_hash_map<string, int> delay2ndshot, delaybooster;
    for (const auto& spec : specs) {
        const auto& vp        = vax_params(vaxset, spec.vax_name);
        delay2ndshot[spec.vax_name] = vp.delay2ndshot;
        delaybooster[spec.vax_name] = vp.delaybooster;
    }

    // doses available today per brand from the spread curve
    vector<int> doses_today(specs.size());
    for (size_t i = 0; i < specs.size(); ++i)
        doses_today[i] = static_cast<int>(
            std::floor(sched.spreadfunc(today) * (float)specs[i].starting_doses));

    // eligible: correct agegrp AND (unexposed OR recovered >= 14 days ago)
    vector<size_t> eligible;
    eligible.reserve(pop.popn / 4);

    for (size_t p = 1; p <= pop.popn; ++p) {

        bool in_filter = false;
        for (const Agegrp& fg : sched.filtervec)
            if (pop.agegrp[p] == fg) { in_filter = true; break; }
        if (!in_filter) continue;

        bool unexposed = (pop.status[p] == Stat::Unexposed);
        bool recovered = (pop.status[p] == Stat::Recovered)
                      && (pop.recovday_count[p] > 0)
                      && (pop.recovday[p][zidx(pop.recovday_count[p])] < today - 14);

        if (unexposed || recovered)
            eligible.push_back(p);
    }

    std::shuffle(eligible.begin(), eligible.end(), xo::get_gen());

    doshots(today, sched, vaxset, vaxlist,
            doses_today, delay2ndshot, delaybooster,
            eligible, pop, series);
}

// ---------------------------------------------------------------
// vaccinate  (called once per simulation day)
// ---------------------------------------------------------------

void vaccinate(int today,
               VaxSchedSet& schedset,
               const VaxSet& vaxset,
               const MapEnum<uint8_t>& vaxlist,
               PopData& pop,
               HistorySeries& series)
{
    for (auto& [name, sched] : schedset.schedules) {
        (void)name;
        vaccinate_sched(today, sched, vaxset, vaxlist, pop, series);
    }
}
