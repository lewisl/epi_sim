#include "lib_includes.h"
#include "parameters.h"
#include "population.h"

struct HistorySeries;

// helpers
static const VaxParams& vax_params(const VaxSet& vaxset, Vax vax);

static size_t spec_index(const vector<PerVaxSpec>& specs, Vax vax);

static void doshots(
        int today,
        VaxSched& sched,
        const VaxSet& vaxset,
        vector<int>& doses_today,
        const absl::flat_hash_map<uint8_t, int>& delay2ndshot,
        const absl::flat_hash_map<uint8_t, int>& delaybooster,
        vector<size_t>& eligible,
        PopData& pop,
        HistorySeries& series);

void vaccinate(int today,
               VaxSchedSet& schedset,
               const VaxSet& vaxset,
               PopData& pop,
               HistorySeries& series);
