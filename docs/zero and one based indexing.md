### one-based indexed arrays and index values
- RuntimeEnum Condition
  - 0 is "uninfected" and not used to access parameter arrays
- RuntimeEnum Status
  - 0 is "none"; not used as an index
- RuntimeEnum Agegrp
  - 0 is "unknown"; should not be used as an index
- RuntimeEnum Vaxstatus
  - 0 is "none", which is a valid status
  - Need to check on use as an index
- Runtime Enum Progressionmap
  - specifically meant to be used as a zero-based index in the progression functions
  - valid_nums are in 0..5

#### PopData
- all data vectors have size population count + 1
- popn is the actual count of the population
- popz is the size (length) of the vectors