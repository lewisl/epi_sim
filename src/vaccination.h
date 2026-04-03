#include "lib_includes.h"
#include "parameters.h"
#include "population.h"

struct HistorySeries;

// helpers
static const VaxParams& vax_params(const VaxSet& vaxset, const string& name);

static size_t spec_index(const vector<PerVaxSpec>& specs, const string& name);

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
        HistorySeries& series);

void vaccinate(int today,
               VaxSchedSet& schedset,
               const VaxSet& vaxset,
               const MapEnum<uint8_t>& vaxlist,
               PopData& pop,
               HistorySeries& series);
