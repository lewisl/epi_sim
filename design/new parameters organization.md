# New Parameters Organization

1. Clean up sample_parameters to use it for essential working, valid examples of each type of .json file essential for creating adn running a model. For now this will include all of the .json and .yml files that are directly below /sample_parameters.  I will review this later to assess the file contents.
2. create a new directory in the project called case_parameters. this will hold inputs and outputs for specific model runs. I will manually organize the first pass of this file reorganization and you should examine it.  You will notice that there are only inputs because I have not rerun the cases.
3. We will preserve the current approach to using --config and --seed parameters for running simple cases and writing default serialized outputs to plot_output, pop_output and series_output. this is for learning and testing how to use this application and experimenting with simple cases. this will include a simplified "canonical" example for vaccination that will be in sample_parameters/sample_vaccine_parameters.
4. We will wire a more realistic approach to loading input parameters and directing outputs and wire it into sim.cpp and setup.cpp.  This approach relies on named directory in the project (mostly during development and for testing) or in another user designated directory outside of the project--though either shall be allowed.  to enable the use of these run cases (my name--nothing official as yet) we will have a new input switch called --case.  the argument must be a directory:  either relative to where the code is running (project dir) or a fully qualified path (more reliable).
5. the case directory must be canonical and contain
   inputs/
      and must contain all required input parameter .json files and optional files
         required:  config.json or config_optionaldesc.json or config-optionaldesc.json
                    seed.json (with opptional description forms)
                    socialparams.json (with optional desc)
                    variants.json (with optional desc)
                    geodata-optionaldesc.csv
         optional:  /vaccine_scheduledesctext.json
                    rings.json
    We will continue to use the convention that the config.json contains keys for the other required and optional .json and .csv files and that the paths to these files are relative to the case/inputs directory.
    Further, we will have an case/outputs directory for the 3 types of serialization output. Note this will entail wiring to the plot_output directory for creating and loading the plot title html files.

6. The sample_parameters file becomes a required part of the application and shouild be installed in the directory where the code is run from.  
7. case directories must follow the established convention for the application but can be located where the user wishes. We should have a convenience and that lets the user establish the top level dir for cases. typically this might be done with an environment variable but that is pretty opaque for a lot of users.  so, instead let's have a case.json file in the program directory like this one:
         case.json >
            {
              "case directory": "~/my covid cases"
            }

         as the only key.  this should be an absolute path.  we should allow ~ expansion on macOS and Linux and we'll have a lot more to do for a Windows port so won't worry about it now.

8. a complexity to think through is how to deal with experimental versions where a user has a case but multiple versoins of inputs to try out. for instance--different rings, etc--anything.  we could just punt and require a clean orginization of a case so the users just copies the entire directory and then parties on it.
9. finally, because required directory structures are often a PITA for anyone, we should have a little directory creating utility that assumes the case.json directory but creates the directory and file scaffolding for a case with starting files which are copies of the sample_parameters or:  much better: we code the text of canonical files into the utility function and it spits out what is required without depending on anything else but the executable.-- I like this approach.  this work as epi_sim --case-init "my foobar case".  all it does is verify the locations, creates the necessary directories, and instantiates the necessary files.  This is clearly the starting point for this implementation.
10. We are going to use jsonc format though we will call the files .json so that we can use comments for the canonical samples.   I have setup nlohmann-json loading to ignore the comments so this works with our code.  some editors won't like it without the file types being manually configged, but the advantage of the comments outweights some orangle squiggly lines.