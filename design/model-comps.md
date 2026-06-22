# comparing models and harnesses
1. lewis
2. claude via claude-cli
3. codex via codex plugin
4. kilo-to-gpt5.5
5. kilo using glm-5.2
6. opencode using glm-5.2

## the prompt
Create a plan to complete the code started in r0_simulation.cpp:
- function r0_sim is the wrapper that is called to use the simulation model to estimate r0.
- run_r0_sim actually runs a limited number of days of the model while tracking how many people become infected by the original infected seeded spreaders. People who become infected by them will infect others but those can't be counted to calculate r0.
- run_r0_sim runs a simplified simulation using the data structures in the passed in model struct, which provide the many parameters needed for the simulation. 
- There are two overloads of r0_sim. The first creates a "fake" population with no seasonality.  It calculates r0 with realistic model parameters, but only runs the simulation long enough for the original seeded, infected spreaders to get better or die.
- The second overload is meant to be run mid-stream during an actual simulation run of a real geographic location. You only need to create a minimal stub as some new logic must be created and it is complex to wire it into the larger simulation. Don't complete the second overload. This is technically called rt (infection rate at time 't'). For this wrapper, you should provide a parameter that is the current state of the population from a simulation midstream.  The second overload is for future Rt estimation, not R0. It should remain a minimal compiling stub/TODO for now. Do not complete or wire it into `runsim`.

A couple of points about the first overload:
- the functions spread and progression use a calendar of simulated days starting at 1 and running to the last day depending on user input. It gets the current day of run from the global namespace `sim`.  While the r0_simulation will typically be run indepedently starting on day 1, you can optionally capture the current sim::ds.day and then reset it when the r0 simulation is completed.

This code should roughly follow the Julia language code pasted in below. This provides the semantics of the r0 simulation but caution is needed to follow it:
- The input parameters are functionally the same but there are slight naming differences.
- Julia syntax is significantly different. Use the Julia code below only for semantics. Do not translate syntax mechanically; adapt to the existing C++ data structures and APIs.

In addition to the incomplete functions in r0_simulation.cpp, it will be useful to examine other c++ code:
- the parameter data structures are defined in parameters.h and parameters.cpp;
- function runsim in sim.cpp shows how the complete simulation is put together;
- setup.h and setup.cpp show the definition of the model container and how it is put together.

Carefully create the plan.  The finished r0_simulation.cpp file will only be about 200 lines but it is tricky code:  
1. First inspect the code and create a concise implementation plan. Do not edit code yet.
2. After I provide feedback on the plan, revise the plan if needed, then implement the approved plan. After implementation, build and run the narrowest relevant validation commands. If you encounter build errors, diagnose them from the actual compiler/linker output and continue fixing until `xmake build epi_sim` succeeds or a real blocker is identified.
3. You can build the app target with `xmake build epi_sim`.
4. You can run the successfully built code with `xmake run epi_sim --r0-sim case-1`   (case-1 exists on my machine and is accessible from the current working set of the code).


**Julia langugage code:**
```julia

"""
    r0_sim(popsize, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor=1.0, scale=3)

Simulates r0 or rt. The first method creates a population and tracks how many infections
are caused by first generation spreaders and NOT spreaders who were infected by the
first generation (or later). 
"""
function r0_sim(popsize::Int, progressionset, trvec, infectset, vaxset, 
                socialparams, dovax=false, variant=:base, density_factor=1.0, scale=3)
    if popsize < 200_000
        @warn "Population size should be greater than 200,000 for r0_sim. Proceeding with 200,000."
        popsize = 200_000
    end

    # create simulation population--> this is all we have to do in the front end function
    r0pop = pop_data(popsize, age_dist=AGE_DIST )

    return _run_r0_sim(r0pop, progressionset, trvec, infectset, vaxset, 
                       socialparams, dovax, variant, density_factor, scale)

end


"""
    r0_sim(locdat, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor, scale)

The second method simulates r at time t given the characteristics of the simulation
you are running. This shows how r is affected by public health
measures and the characteristics of the population over time based on your simulation
parameters. This simulates r(t).

It will be called as part of the simulation run in function runsim so that it uses the
context of the current simulation with its input parameters.
"""
function r0_sim(locdat, progressionset, trvec, infectset, vaxset, 
                socialparams, dovax, variant=:base, density_factor=1.0, scale=3)

    # create the simulation population--> this is all we have to do in the front end function
    r0pop = deepcopy(locdat)

    return _run_r0_sim(r0pop, progressionset, trvec, infectset, vaxset, 
                        socialparams, dovax, variant, density_factor, scale)

end

# this internal function does all the work once the simulation population has been created...
function _run_r0_sim(r0pop, progressionset, trvec, infectset, vaxset, 
    socialparams, dovax=false, variant=:base, density_factor=1.0, scale=3)  

        # deref columns for input to spread! and progression!
        c_cond        = r0pop.cond
        c_status      = r0pop.status
        c_agegrp      = r0pop.agegrp
        c_duration    = r0pop.duration
        c_sdcase      = r0pop.sdcase
        c_sickday     = r0pop.sickday
        c_variant     = r0pop.variant
        c_vaxstatus   = r0pop.vaxstatus
        c_recovday    = r0pop.recovday
        c_vaxrcvd     = r0pop.vaxrcvd
        c_vaxday      = r0pop.vaxday
        c_deadday     = r0pop.deadday   # only for progression!

        poprange = 1:length(r0pop)

    # seed spreaders in each age group proportional to age distribution
    cnt_by_agedist = round.(Int, AGE_DIST ./ minimum(AGE_DIST))
    cnt_by_agedist .*= scale # update with scale

    for age in AGEGRPS
        age_idx = findall(r0pop.agegrp .== age) 

        for j in 1:cnt_by_agedist[mapagegrp(age)]
            target = age_idx[j]
            c_status[target] = :infectious
            c_cond[target] = :nil
            c_duration[target] = 1
            push!(c_variant[target], variant)
            push!(c_sickday[target], 1)
        end
    end

    # set infect_idx based on seeding: never update so we measure only 1st gen. spreaders
    gen1_spreaders = findall(r0pop.status .== :infectious)
    gen1_infect_idx = copy(gen1_spreaders) # use for gen1 after progressing each day
    gen1_spreader_cnt = length(gen1_spreaders)
    r0_infected = 0
    indoor_seq = ones(Float64, DURATIONLIM)   # value of 1.0 has no effect

    for simday = 1:DURATIONLIM        # day loop
        all_infect_idx = findall(r0pop.status .== :infectious)

        for spr in gen1_infect_idx  # only spreaders from the gen1 infected pool
            cnt_newly_infected = spread!(r0pop, spr, simday, [], socialparams,   # THIS NO LONGER WORKS WITH SPREAD!
                            infectset, vaxset, density_factor, indoor_seq, poprange)

            r0_infected += cnt_newly_infected
        end

        for p in all_infect_idx  # progress all who are currently infected
            progression!(r0pop, p, infectset, progressionset, vaxset, dovax, trvec)
        end

        # of the gen1 infected, who is still infected? (some will have progressioned to recovered or dead)
        gen1_infect_idx = filter(x -> r0pop.status[x] == :infectious, gen1_infect_idx)
    end

    r0 =  r0_infected / gen1_spreader_cnt  
    return r0
end


function r0_table(n=6, cfstart = 0.9, tfstart = 0.3; popsize::Int, progressionset, trvec, infectset, vaxset, 
                    socialparams, dovax=false, variant=:base, density_factor=1.0, scale=3)
    tbl = zeros(n+1,n+1)
    cfiter = [cfstart + (i-1) * .1 for i=1:n]
    tfiter = [tfstart + (i-1) * 0.05 for i=1:n]

    for (j,cf) in enumerate(cfiter)
        for (i,tf) = enumerate(tfiter)

                #=
                need to create new socialparams shifting ranges for each iteration
                =#



            tbl[i+1,j+1] = r0_sim(popsize, progressionset, trvec, infectset, vaxset, 
                            socialparams, dovax, variant, density_factor, scale)
        end
    end
    tbl[1, 2:n+1] .= cfiter
    tbl[2:n+1, 1] .= tfiter
    tbl[:] = round.(tbl, digits=2)
    display(tbl)
    return tbl
end

#=
approximate r0 values from model
using default age distribution
model selects a contact_factor, c_f, based on age and infectious case
model selects a touch_factor, t_f, based on age and condition (includes unexposed and recovered)
r0 depends on the selection of both c_f and t_f
Note: simulation uses samples so generated values will vary

           c_f
  tf       1.1   1.2      1.3   1.4     1.5   1.6    1.7    1.8    1.9    2.0
           ----------------------------------------------------------
     0.18 | 0.38| 0.38 | 0.42 | 0.46 | 0.49 | 0.51 | 0.55 | 0.57 | 0.59 | 0.64
     0.23 | 0.47| 0.47 | 0.49 | 0.55 | 0.64 | 0.65 | 0.68 | 0.68 | 0.73 | 0.77
     0.28 | 0.53| 0.61 | 0.62 | 0.65 | 0.69 | 0.73 | 0.79 | 0.82 | 0.83 | 0.88
     0.33 | 0.61| 0.66 | 0.7  | 0.79 | 0.8  | 0.83 | 0.9  | 0.95 | 0.99 | 1.04
     0.38 | 0.7 | 0.74 | 0.85 | 0.84 | 0.94 | 0.98 | 1.04 | 1.08 | 1.11 | 1.17
     0.43 | 0.8 | 0.85 | 0.89 | 0.93 | 1.03 | 1.11 | 1.16 | 1.2  | 1.27 | 1.34
     0.48 | 0.88| 0.91 | 0.99 | 1.03 | 1.16 | 1.23 | 1.26 | 1.32 | 1.42 | 1.47
     0.53 | 0.97| 1.06 | 1.08 | 1.18 | 1.26 | 1.27 | 1.42 | 1.47 | 1.52 | 1.61
     0.58 | 1.01| 1.09 | 1.17 | 1.25 | 1.33 | 1.43 | 1.52 | 1.52 | 1.68 | 1.76
     0.63 | 1.11| 1.2  | 1.25 | 1.38 | 1.42 | 1.5  | 1.65 | 1.75 | 1.78 | 1.95

=#|
```

## replies and results