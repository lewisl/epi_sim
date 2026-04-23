Here's the status list you can copy:

---

## epi_sim — Outstanding Work Items

### ✅ Completed (this session)
1. **`spread.cpp`** — `indoor_factor` hardcoded to `1.0` in both `how_many_contacts` and `istouched` *(temporary — matches Julia's current bug)*
2. **`progression.cpp`** — Breakpoint lookup uses `p_duration + 1` throughout to align C++ 0-based duration with Julia's 1-based breakpoint keys
3. **`parameters.cpp`** — Condition probability ordering fixed: `nlohmann::json` iterates alphabetically (`mild < nil < severe < sick`); now loads explicitly by `Condition::names` order
4. **`progression.cpp`** — `redistribute_probability` receives `p_duration + 1` so `DURATIONLIM` branch fires correctly at last breakpoint; closes latent OOB hazard on `sendrisk`
5. **`progression.cpp` / `riskfactor`** — Removed 0.97 clamp; now clamps to `[0.0, 1.0]` so no-immunity cases are a true no-op in `redistribute_probability`
6. **C++ vs Julia output** — Death counts now match within epsilon across all age groups ✅

---

### 🔧 Still To Do

#### Both C++ and Julia
1. **`recoveffect` — wrong status check**
   - Currently checks `status == Recovered` (spread) or `status == Infectious` (progression)
   - Fix: check `variant_count > 1` to detect prior infection
   - Affects both `spread.cpp` (`isinfected`) and `progression.cpp`
   - Same bug exists in Julia — fix both

2. **`indoor_factor` — Julia date boundary bug**
   - Julia's indoor sequence logic has a bug that always returns 1.0
   - C++ `spread.cpp` is temporarily hardcoded to 1.0 to match
   - Fix Julia first, then remove the C++ hardcode and use real `indoor_seq`

#### C++ Only
3. **`riskadjust`** — loaded from JSON into `ProgressionFactors` but never applied in `progression.cpp`; Julia applies it — needs implementation
4. **Simulation accounting and output** — series history, summary statistics, locale aggregation
5. **Vaccination effect** — `vaxeff` hardcoded to `1.0f` throughout; `vaxeffect()` exists but not wired in

#### Validate After Fixes
6. **Re-tune input parameters** — once `recoveffect` fires correctly during progression, reinfected people will progress less severely; death/severity counts will shift and clinical parameters (`immunehalflife`, `immstrength`, `csig`, `decay_lower`) may need adjustment

---
