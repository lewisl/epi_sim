### Here is what the parameters look like after being loaded.

```
=== APPORTION DEBUG (n=95626) ===
  parts[0] = round(95626 * 0.251) = 24002
  parts[1] = round(95626 * 0.271) = 25915
  parts[2] = round(95626 * 0.255) = 24385
  parts[3] = round(95626 * 0.184) = 17595
  parts[4] = round(95626 * 0.039) = 3729
Total before adjustment: 95626
Difference (total - n): 0
Total after adjustment: 95626
Expected (n): 95626
Match: YES
=== END APPORTION DEBUG ===


Total counts: 95627 popz: 95627 popn: 95626
agegrp tmp size: 95627 popz: 95627
Initialized values, at index 0:  at index 1:  at index popn: 
New values, at index 0: unknown at index 1: age0_19 at index popn: age80_up
    fips    county            city              state   sizecat   pop       density   anchor      
 0: 6075    San Francisco     San Francisco     CA      2         881549    1.04      2020-02-01  
 1: 53033   Seattle           Seattle           WA      2         2252782   0.932     2020-02-01  
 2: 36061   New York          New York          NY      1         8336817   1.25      2020-02-01  
 3: 39035   Cuyahoga          Cleveland         OH      2         1235072   0.912     2020-02-01  
 4: 48113   Dallas            Dallas            TX      2         2635516   0.921     2020-02-01  
 5: 39151   Stark             Canton            OH      3         370606    0.9       2020-02-01  
 6: 34013   Essex             Newark            NJ      3         798975    0.943     2020-02-01  
 7: 13089   DeKalb            Atlanta           GA      2         1063937   0.909     2020-02-01  
 8: 17167   Sangamon          Springfield       IL      3         194672    0.901     2020-02-01  
 9: 38015   Burleigh          Bismarck          ND      3         95626     0.904     2020-02-01  
10: 4013    Maricopa          Phoenix           AZ      1         4485414   0.91      2020-03-01  
11: 42003   Allegheny         Pittsburgh        PA      2         1216045   0.934     2020-02-01  
12: 27053   Hennepin          Minneapolis       MN      2         1265843   0.956     2020-02-01  
13: 31055   Douglas           Omaha             NE      2         571327    0.915     2020-02-01  
14: 8031    Denver            Denver            CO      2         727211    0.926     2020-02-01  

 0: none    
 1: base    
 2: alpha   
 3: delta   
 4: omicron_ba1
 5: omicron_ba2
 6: omicron_ba4_5

========== vector<InfectParams> =============
 ==== infectparams of variant none ====
  sendrisk=[],
  recvrisk=[],
  base=1.00,   halflife=0
 ==== infectparams of variant base ====
  sendrisk=[0, 0.3, 0.65, 0.75, 0.85, 0.85, 0.75, 0.7, 0.65, 0.6, 0.5, 0.2, 0.1, 0.1, 0.1, 0.05, 0.05, 0, 0, 0, 0, 0, 0, 0, 0],
  recvrisk=[0.1, 0.39, 0.44, 0.54, 0.56],
  base=1.00,   halflife=360
 ==== infectparams of variant alpha ====
  sendrisk=[],
  recvrisk=[],
  base=1.10,   halflife=360
 ==== infectparams of variant delta ====
  sendrisk=[],
  recvrisk=[],
  base=1.20,   halflife=360
 ==== infectparams of variant omicron_ba1 ====
  sendrisk=[0, 0.445, 0.975, 1.125, 1.275, 1.275, 1.125, 1.05, 0.975, 0.9, 0.75, 0.5, 0.1, 0.05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  recvrisk=[0.1, 0.39, 0.44, 0.54, 0.56],
  base=1.20,   halflife=360
 ==== infectparams of variant omicron_ba2 ====
  sendrisk=[0, 0.545, 1.175, 1.325, 1.475, 1.475, 1.325, 1.25, 1.075, 0.9, 0.75, 0.5, 0.1, 0.05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  recvrisk=[0.1, 0.39, 0.44, 0.54, 0.56],
  base=1.30,   halflife=360
 ==== infectparams of variant omicron_ba4_5 ====
  sendrisk=[0, 0.545, 1.175, 1.325, 1.475, 1.475, 1.325, 1.25, 1.075, 0.9, 0.75, 0.5, 0.1, 0.05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  recvrisk=[0.1, 0.39, 0.44, 0.54, 0.56],
  base=1.40,   halflife=360
========== End vector<InfectParams> =============

gammashape: 1 indoor_uplift: 1.1
contactfactors (4x5):
              age0_19   age20_39   age40_59   age60_79   age80_up
       nil       1.10       2.10       2.10       1.70       1.00 
      mild       1.10       2.00       2.00       1.60       0.90 
      sick       0.70       1.00       1.00       0.70       0.60 
    severe       0.50       0.60       0.60       0.50       0.50 

touchfactors (6x5):
              age0_19   age20_39   age40_59   age60_79   age80_up
 unexposed       0.55       0.63       0.61       0.41       0.35 
 recovered       0.55       0.63       0.61       0.41       0.35 
       nil       0.55       0.63       0.61       0.41       0.35 
      mild       0.55       0.62       0.58       0.41       0.28 
      sick       0.28       0.35       0.30       0.18       0.18 
    severe       0.18       0.18       0.18       0.18       0.18 



=== ProgressionSet ===
Total variants: 6

Variant: none
  Progression Factors:
    riskadjust: <empty>
    vaxhalflifeadjust:
      JnJ: 0.50
      Moderna: 0.90
      Pfizer: 0.80
  Progression Tree:
    Age group: age0_19
      Day 5: 4 conditions
        nil: [0.00, 0.40, 0.50, 0.10, 0.00, 0.00]
        mild: [0.00, 0.30, 0.60, 0.10, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 1.00, 0.00, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 9: 4 conditions
        nil: [0.90, 0.00, 0.00, 0.10, 0.00, 0.00]
        mild: [0.40, 0.00, 0.60, 0.00, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 0.95, 0.05, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 14: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.85, 0.00, 0.00, 0.12, 0.03, 0.00]
        severe: [0.69, 0.00, 0.00, 0.00, 0.30, 0.01]
      Day 19: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 1.00, 0.00, 0.00]
        severe: [0.89, 0.00, 0.00, 0.00, 0.11, 0.00]
      Day 25: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.98, 0.00, 0.00, 0.00, 0.00, 0.02]
        severe: [0.91, 0.00, 0.00, 0.00, 0.00, 0.09]
    Age group: age20_39
      Day 5: 4 conditions
        nil: [0.00, 0.20, 0.70, 0.10, 0.00, 0.00]
        mild: [0.00, 0.15, 0.85, 0.00, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 1.00, 0.00, 0.00]
        severe: [0.00, 0.00, 0.00, 1.00, 0.00, 0.00]
      Day 9: 4 conditions
        nil: [0.90, 0.00, 0.00, 0.10, 0.00, 0.00]
        mild: [0.85, 0.00, 0.00, 0.15, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 0.90, 0.10, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 14: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.83, 0.00, 0.00, 0.10, 0.07, 0.00]
        severe: [0.47, 0.00, 0.00, 0.00, 0.51, 0.01]
      Day 19: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.92, 0.00, 0.00, 0.00, 0.07, 0.01]
        severe: [0.92, 0.00, 0.00, 0.00, 0.07, 0.01]
      Day 25: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.96, 0.00, 0.00, 0.00, 0.00, 0.04]
        severe: [0.96, 0.00, 0.00, 0.00, 0.00, 0.04]
    Age group: age40_59
      Day 5: 4 conditions
        nil: [0.00, 0.20, 0.70, 0.10, 0.00, 0.00]
        mild: [0.00, 0.15, 0.85, 0.00, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 1.00, 0.00, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 9: 4 conditions
        nil: [0.90, 0.00, 0.00, 0.10, 0.00, 0.00]
        mild: [0.85, 0.00, 0.05, 0.10, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 0.90, 0.10, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 14: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [0.90, 0.00, 0.00, 0.10, 0.00, 0.00]
        sick: [0.85, 0.00, 0.00, 0.14, 0.01, 0.00]
        severe: [0.78, 0.00, 0.00, 0.00, 0.21, 0.02]
      Day 19: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.86, 0.00, 0.00, 0.00, 0.13, 0.02]
        severe: [0.86, 0.00, 0.00, 0.00, 0.13, 0.02]
      Day 25: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.96, 0.00, 0.00, 0.00, 0.00, 0.04]
        severe: [0.96, 0.00, 0.00, 0.00, 0.00, 0.04]
    Age group: age60_79
      Day 5: 4 conditions
        nil: [0.00, 0.15, 0.60, 0.25, 0.00, 0.00]
        mild: [0.00, 0.00, 0.70, 0.30, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 1.00, 0.00, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 9: 4 conditions
        nil: [0.62, 0.00, 0.00, 0.38, 0.00, 0.00]
        mild: [0.50, 0.00, 0.25, 0.25, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 0.78, 0.22, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 14: 4 conditions
        nil: [0.80, 0.10, 0.10, 0.00, 0.00, 0.00]
        mild: [0.80, 0.00, 0.15, 0.05, 0.00, 0.00]
        sick: [0.80, 0.00, 0.00, 0.10, 0.10, 0.00]
        severe: [0.17, 0.00, 0.00, 0.00, 0.71, 0.12]
      Day 19: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.81, 0.00, 0.00, 0.00, 0.13, 0.06]
        severe: [0.81, 0.00, 0.00, 0.00, 0.13, 0.06]
      Day 25: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.76, 0.00, 0.00, 0.00, 0.00, 0.24]
        severe: [0.69, 0.00, 0.00, 0.00, 0.00, 0.31]
    Age group: age80_up
      Day 5: 4 conditions
        nil: [0.00, 0.10, 0.50, 0.40, 0.00, 0.00]
        mild: [0.00, 0.10, 0.50, 0.40, 0.00, 0.00]
        sick: [0.00, 0.10, 0.50, 0.40, 0.00, 0.00]
        severe: [0.00, 0.00, 0.00, 0.40, 0.60, 0.00]
      Day 9: 4 conditions
        nil: [0.50, 0.00, 0.00, 0.50, 0.00, 0.00]
        mild: [0.00, 0.00, 0.40, 0.60, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 0.60, 0.40, 0.00]
        severe: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
      Day 14: 4 conditions
        nil: [0.70, 0.00, 0.30, 0.00, 0.00, 0.00]
        mild: [0.70, 0.00, 0.00, 0.30, 0.00, 0.00]
        sick: [0.70, 0.00, 0.00, 0.10, 0.20, 0.00]
        severe: [0.12, 0.00, 0.00, 0.00, 0.67, 0.21]
      Day 19: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.00, 0.00, 0.00, 0.00, 1.00, 0.00]
        severe: [0.49, 0.00, 0.00, 0.00, 0.24, 0.27]
      Day 25: 4 conditions
        nil: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        mild: [1.00, 0.00, 0.00, 0.00, 0.00, 0.00]
        sick: [0.68, 0.00, 0.00, 0.00, 0.00, 0.32]
        severe: [0.68, 0.00, 0.00, 0.00, 0.00, 0.32]

Variant: base
  Progression Factors:
    riskadjust: [1.00, 1.00, 1.00, 1.10, 1.10, 1.10]
    vaxhalflifeadjust:
      JnJ: 0.50
      Moderna: 0.90
      Pfizer: 0.80
  Progression Tree:
    Tree: <empty>

Variant: alpha
  Progression Factors:
    riskadjust: [1.00, 1.00, 1.00, 1.10, 1.10, 1.10]
    vaxhalflifeadjust:
      JnJ: 0.90
      Moderna: 0.90
      Pfizer: 0.90
  Progression Tree:
    Tree: <empty>

Variant: delta
  Progression Factors:
    riskadjust: [1.10, 1.05, 1.05, 0.90, 0.70, 0.60]
    vaxhalflifeadjust:
      JnJ: 0.50
      Moderna: 0.90
      Pfizer: 0.80
  Progression Tree:
    Tree: <empty>

Variant: omicron_ba1
  Progression Factors:
    riskadjust: [1.10, 1.05, 1.05, 0.90, 0.50, 0.60]
    vaxhalflifeadjust:
      JnJ: 0.50
      Moderna: 0.90
      Pfizer: 0.80
  Progression Tree:
    Tree: <empty>

Variant: omicron_ba2
  Progression Factors:
    riskadjust: [1.10, 1.05, 1.05, 0.90, 0.50, 0.60]
    vaxhalflifeadjust:
      JnJ: 0.50
      Moderna: 0.90
      Pfizer: 0.80
  Progression Tree:
    Tree: <empty>

=== End ProgressionSet ===



=== VaxSet ===
Total vaccines: 3
Shot types: first, full, booster

  Vaccine: Pfizer
    Required shots: 2
    Delay 2nd shot: 21 days
    Delay booster: 160 days
    Half-life: 360 days
    Full effect days: 14
    Day 1 effect: 0.65
    Infect factor by variant:
      base           : 0.90
      alpha          : 0.90
      delta          : 0.85
      omicron_ba1    : 0.75
      omicron_ba2    : 0.75
      omicron_ba4_5  : 0.75
    Effectiveness by shot type:
      first:
        base           : 0.90
        alpha          : 0.90
        delta          : 0.70
        omicron_ba1    : 0.65
        omicron_ba2    : 0.65
        omicron_ba4_5  : 0.65
      full:
        base           : 0.94
        alpha          : 0.94
        delta          : 0.80
        omicron_ba1    : 0.85
        omicron_ba2    : 0.85
        omicron_ba4_5  : 0.85
      booster:
        base           : 0.94
        alpha          : 0.94
        delta          : 0.80
        omicron_ba1    : 0.85
        omicron_ba2    : 0.85
        omicron_ba4_5  : 0.85

  Vaccine: Moderna
    Required shots: 2
    Delay 2nd shot: 21 days
    Delay booster: 160 days
    Half-life: 360 days
    Full effect days: 14
    Day 1 effect: 0.65
    Infect factor by variant:
      base           : 0.90
      alpha          : 0.90
      delta          : 0.85
      omicron_ba1    : 0.75
      omicron_ba2    : 0.75
      omicron_ba4_5  : 0.75
    Effectiveness by shot type:
      first:
        base           : 0.92
        alpha          : 0.92
        delta          : 0.85
        omicron_ba1    : 0.60
        omicron_ba2    : 0.60
        omicron_ba4_5  : 0.60
      full:
        base           : 0.95
        alpha          : 0.95
        delta          : 0.88
        omicron_ba1    : 0.75
        omicron_ba2    : 0.75
        omicron_ba4_5  : 0.75
      booster:
        base           : 0.95
        alpha          : 0.95
        delta          : 0.88
        omicron_ba1    : 0.80
        omicron_ba2    : 0.80
        omicron_ba4_5  : 0.80

  Vaccine: JnJ
    Required shots: 1
    Delay 2nd shot: 0 days
    Delay booster: 160 days
    Half-life: 360 days
    Full effect days: 14
    Day 1 effect: 0.65
    Infect factor by variant:
      base           : 0.85
      alpha          : 0.85
      delta          : 0.75
      omicron_ba1    : 0.65
      omicron_ba2    : 0.65
      omicron_ba4_5  : 0.65
    Effectiveness by shot type:
      first:
        base           : 0.88
        alpha          : 0.88
        delta          : 0.65
        omicron_ba1    : 0.75
        omicron_ba2    : 0.75
        omicron_ba4_5  : 0.75
      full:
        base           : 0.88
        alpha          : 0.88
        delta          : 0.65
        omicron_ba1    : 0.75
        omicron_ba2    : 0.75
        omicron_ba4_5  : 0.75
      booster:
        base           : 0.88
        alpha          : 0.88
        delta          : 0.65
        omicron_ba1    : 0.75
        omicron_ba2    : 0.75
        omicron_ba4_5  : 0.75

=== End VaxSet ===


============ VaxList ==============
 0: Pfizer  
 1: Moderna 
 2: JnJ     
============ End Vaxlist ==========


=== VaxSched ===
Vaccines included: 3
    Vaccine: Pfizer
      Mix: 0.50
      Starting doses: 40000
      Pct 2nd shot: 0.90
      Pct boost: 0.60
      Alternates: Moderna, JnJ
    Vaccine: Moderna
      Mix: 0.45
      Starting doses: 36000
      Pct 2nd shot: 0.90
      Pct boost: 0.60
      Alternates: Pfizer, JnJ
    Vaccine: JnJ
      Mix: 0.05
      Starting doses: 4000
      Pct 2nd shot: 0.00
      Pct boost: 0.40
      Alternates: Pfizer, Moderna

  Day range: [350, 700]
  Target pct: 0.95
  Filter: age80_up, age60_79
  Shot mode: all
  Pattern: [0.00, 0.02, 0.05, 0.10, 0.15, 0.19, 0.21, 0.16, 0.08, 0.03, 0.01, 0.00]
  Spread func: null
=== End VaxSched ===
```