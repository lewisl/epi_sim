#include <algorithm>

#include "parameters.h"
#include "population.h"
#include "r0_simulation.h"
#include "setup.h"
#include "traits.h"
#include <fmt/base.h>
#include <fmt/format.h> // only get what I use: about 12k in the executable!
#include <fmt/ranges.h> // for printing containers like vector
#include <fmt/ostream.h> // to use ostream file handles and << >> operators



/*
    r0_sim(popsize, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor=1.0, scale=3)

Simulates r0 or rt. The first method creates a population and tracks how many infections
are caused by first generation spreaders and NOT spreaders who were infected by the
first generation (or later). 
*/

/*.    mp -- modelparams
      .geodata = std::move(geodata),
      .variants = std::move(variants),
      .infectparams = std::move(infectparams),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = std::move(socialdata),
      .vaxset = std::move(vaxdata),
      .vaxschedset = std::move(vaxschedset),
      .ringtraits = std::move(ringtraits),
*/
void r0_sim(Model & model) {
  // extract required inputs
  int ndays = model.ndays;
  int locale = model.locale;
  bool do_rings = model.do_rings;
  bool dovax = model.dovax;
  int popn = 200'000;          // julia popsize
  PopData & pop = model.pop;          // julia locdat
  VaxSet & vaxset = model.mp.vaxset;  
  float density_factor = 1.0;
  int scale = 3;
  vector<double> age_dist = AGE_DIST;
  ProgressionSet & progressionset = model.mp.progressionset;
  vector<InfectParams> & infectparams = model.mp.infectparams;  // julia infectset
  SocialParams & socialparams = model.mp.socialdata;
  array<float, 6> & trvec = model.mp.trvec;

  Variant use_variant = 1;  // julia variant
  PopData r0_pop = PopData(popn, age_dist);

  fmt::println("ndays: {}  |  locale: {}  |  do_rings: {}  |", ndays, locale, do_rings);

  _run_r0_sim(r0_pop, progressionset, trvec, infectparams, vaxset, socialparams, dovax);

}

void _run_r0_sim(PopData & r0pop, ProgressionSet & progressionset, array<float, 6> trvec,vector<InfectParams> & infectset, 
  VaxSet & vaxset, SocialParams & socialparams, bool dovax, Variant variant, float density_factor, int scale) {

    AllSeries r0series(DURATIONLIM, r0pop, Variant::names.size(),
                   1, 1);

    int popn = r0pop.agegrp.size() - 1;
    // seed spreaders in each age group proportional to age distribution
    array<int, 5> cnt_by_agedist;  // TODO should extract the count of AGE_DIST and not assume it
    uint8_t i=0;
    for (auto val : AGE_DIST) {
      cnt_by_agedist[i] = (val / std::ranges::min(AGE_DIST)) * scale; // bad boy: implicit conversion
      i++;
      }


    for (size_t p = 1; p <= r0pop.popn; ++p) {
      auto person = r0pop.agent(p);
      switch (person.agegrp()) 
      {
        case AGE0_19: {
          if (cnt_by_agedist[0] > 0) 
          {person.make_sick(variant,r0series); cnt_by_agedist[0]--; break;}
          else break;
          }
        case AGE20_39: {
          if (cnt_by_agedist[1] > 0) 
          {person.make_sick(variant,r0series); cnt_by_agedist[1]--; break;}
          else break;
          }
        case AGE40_59: {
          if (cnt_by_agedist[2] > 0) 
          {person.make_sick(variant,r0series); cnt_by_agedist[2]--; break;}
          else break;
          }
        case AGE60_79: {
          if (cnt_by_agedist[3] > 0) 
          {person.make_sick(variant,r0series); cnt_by_agedist[3]--; break;}
          else break;
          }
        case AGE80_UP: {
          if (cnt_by_agedist[4] > 0) 
          {person.make_sick(variant,r0series); cnt_by_agedist[4]--; break;}
          else break;
          }
        default:
          throw std::runtime_error("Invalid agegrp for seeding r0_simulation.");
      }  
    }


  // intermediate tests
  fmt::println("min of AGE_DIST: {}", std::ranges::min(AGE_DIST));  // OK
  fmt::println("{}",  AGE_DIST);  // OK
  fmt::println("{}", cnt_by_agedist);  // OK

  // count infected by agegrp that were seeded
  array<int, 5> age_seeded_cnt {};
  for (size_t p = 1; p <= r0pop.popn; ++p) {
      auto person = r0pop.agent(p); 
      switch (person.agegrp()) 
      {
        case AGE0_19: {
          if (person.status() == INFECTIOUS) 
          {age_seeded_cnt[0]++; break;}
          else break;
          }
        case AGE20_39: {
          if (person.status() == INFECTIOUS) 
          {age_seeded_cnt[1]++; break;}
          else break;
          }
        case AGE40_59: {
          if (person.status() == INFECTIOUS) 
          {age_seeded_cnt[2]++; break;}
          else break;
          }
        case AGE60_79: {
          if (person.status() == INFECTIOUS) 
          {age_seeded_cnt[3]++; break;}
          else break;
          }
        case AGE80_UP: {
          if (person.status() == INFECTIOUS) 
          {age_seeded_cnt[4]++; break;}
          else break;
          }
        default:
          throw std::runtime_error("Invalid agegrp for seeding r0_simulation.");
    }
  }

    fmt::println("seeded by age group: {}", age_seeded_cnt) ; // OK

}