### one-based indexed arrays and index values
- MapEnum Condition
  - 0 is "uninfected" and not used to access parameter arrays
- MapEnum Status
  - 0 is "none"; not used as an index
- MapEnum Agegrp
  - 0 is "unknown"; should not be used as an index
- MapEnum Variant
  - 0 is "none"; not used as an index and generally is an error if it occurs during simulation
  - 1 should always be "base"; it is required and corresponds to a real variant; it is often used to derive other variant parameters
- MapEnum Vaxstatus
  - 0 is "none", which is a valid status
  - Need to check on use as an index
- Runtime Enum Progressionmap
  - specifically meant to be used as a zero-based index in the progression functions
  - valid_nums are in 0..5

#### PopData
- all data vectors have size population count + 1
- popn is the actual count of the population
- popz is the size (length) of the vectors
- when looping through people use indices 1..popn. popz is the physical size of the vectors = popn + 1

#### DayData series
- all vectors in DayData accumulate statistics during the simulation per day. these vectors are +1 larger than the number of days to be held. indexing across days should be in 1..model.ndays. for an example here is the start of the day loop in function runsim:
        ```cpp
        // day loop
        for (int d_i = 1; d_i <= model.ndays; ++d_i) { <loop body follows...>
        ```
- 0 is not used; there is no day zero.  day 1 is the first day of the simulation.  of course,c++ vectors and arrays are zero-indexed so care must be taken when accessing day indexed elements and looping across days.