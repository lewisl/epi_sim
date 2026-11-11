test_disease_modeling
- correct the total AI to use 5 coordinate selection: trait, phase, trait_value, ring, agegrp
- need test for recoveffect

overall:
- separation by focus is good
- construction of artifacts need to be reviewed for compatibility with latest good
- ability to save artifact with input flag is desirable
- but there is too much testing of isolated fragmentary fixtures and not enough testing of the code is the source files
- existing tests tend to focus too much on testing happy path, which is important because it is good for regression testing to make sure the developer didn't break code, but there is not enough testing for handling of bad inputs and bad logic in spread and progression that produces wrong mathematical results
- a lot of the tests focused on the early development stages and emphasized atomic unit tests.  now we have a lot of complicated code that needs more macro testing on the process and its outputs. this are important but failure is hard to diagnose. some linkage between macro testing and suggested unit tests would be helpful.

test_parameters
- loaders of parameter json files have been changed to make sure corresponding trait is virtual 1-indexed. make sure tests comply
- we have checks on the formatting of the json files, which is good--though most of these tests are builtin to the loading process.  few users will run the tests so we need to tell them if inputs are malformed at load time with suitablke throw messages.  this work was done but possibly not fully up to date
- we are not testing the result:  the created data structures for each parameter category:  vaccines, vaxschedule, rings, variants--including social parameters must meet the requirements for those data structures to be used by the simulation code

test_plot
- focus more on intended results:  loading of data vectors has to meet plotly json requirment--and actually plot successfully
- plot test might need to include png file of correct plot output for plot fixture to verify the plot is actually correct.
- test access of correct history vectors

test_pop_serialize
- haven't run this in a while
- what's missing is comparison of selection and output to artifact of valid output

test_runsim
- this doesn't seem to do much good.
- what it should do:  setup through runsim function with a very simplified case using the rundir function of the tui.
- we are testing the pieces inm other tests so this just makes sure the flow and some of the preliminary steps of runsim haven't broken
- using a simple and defined case, the inputs can be matched to a test fixture of the terminal output and the files im the case dir output subdir
- in other words as written the test tests artificial fixtures and not the real code, which a devreloper may have unintentionally broken

test_series
- this mostly tests code that doesn't exist any more
- this should focus on building a valid instance of class Histories: including the collection of vectors in the right order, with matching meta members and correct map access to the vectors and the HistorySelector and HistorySelectorSet to access vectors
- it should also test the totaling of simulation history into the correct total vectors

test_setup
- focus on tesrting the overall setup flow up to runsim to see if the setups works to enable valid simulation runs
- overlaps with testing the templates and the tui.   directory handing was made more structured and that should be tested with setup a case dir and running cases and setting up dir and running the single case in a dir

test_traits
- this is fundamental to making sure inputs load into the traits; that PopData is initialized with correct values; and that selection of history vectors works
- some challenges are around using correct types for traits and proper initializing of trait literals used for initialization, for building Histories (vectors, meta, and maps), for selecting history vectors using string selectors and the history vector maps
- to avoid duplication this might focus more on bad constructors for the traits and overlap with setup and parameter loading
- also a correct focus on the required 0 value of a trait so that the trait works like a virtual 1-based system, except for Vaxstatus (which has changed)

test_vaccination
- this is complex code. starting with the inputs of vaccines  and vax schedule parameters focus on:
1. the vaccine shots are delivered to the correct number of people based om the vaccine supply and the vax schedules
2. the vaccines have the effects the follow from their parameters.  the effects are modeled in disease_modeling to affect spread and progression
- most of the existing tests can be used but they focus on delivery of shots
- there are no tests for effect of vaccines corresponding to the input parameters.

test_social_distancing
- there are no existing tests for this and it is important
- somewhat related are testing seed cases:  without correct seeding the simulation can't even start
