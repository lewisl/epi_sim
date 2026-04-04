## TODO
- use comments in json files!
- serialization of popdata
- test and trace
- social distancing cases
- r0 calculations
- implement rings
- clean up test:  update for latest API changes.
- Decide what to do about Touch factors where we ignore people who are sick because they can't get infected again--this
    only matters if we model viral load and if contact affects progression to more severe condition

## Done
- serialization of series
- make key series functions class functions for HistorySeries. Result:  major speed up!
- remove debug counter in hotloop or comment out or use debug switch
- clean up series name containers ease maintainability and reduce order dependence
