```
variant (e.g., "delta", "omicron")
  └─ progression_tree
      └─ agegrp (e.g., "0-19", "20-39", "40-59", "60-79", "80+")
          └─ breakday (e.g., 5, 9, 14, 19, 25)
              └─ condition (nil, mild, sick, severe)
                  └─ transition_vector [recover, nil, mild, sick, severe, dead]
                                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                        6 floats summing to 1.0 (probabilities)
```
