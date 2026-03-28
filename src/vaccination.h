#include "lib_includes.h"
#include "parameters.h"
#include "population.h"

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
        PopData& pop);

void vaccinate(int today,
               VaxSched& sched,
               const VaxSet& vaxset,
               const MapEnum<uint8_t>& vaxlist,
               PopData& pop);