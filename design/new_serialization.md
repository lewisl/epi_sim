For the current code, I’d suggest thinking in terms of three layers:

`1.` Convenience pretty-print to console  
Use `print_agent_pop_table(...)`.

```cpp
print_agent_pop_table(pop, rows, {"status", "agegrp", "variant_hist"}, std::cout, true);
```

- `rows` is the subset of people you want
- last `true` means expand multivalued `_hist` fields across multiple lines
- omit the stream argument to default to `std::cout`

`2.` Pretty-print to a file  
Use the same `print_agent_pop_table(...)`, just pass an `std::ofstream`.

```cpp
std::ofstream report("pop_report.txt");
print_agent_pop_table(pop, rows, {"status", "agegrp", "variant_hist"}, report, true);
```

That gives you the aligned table layout in a file.

`3.` Serialized output  
For full-population file export, the simplest wrapper is still `serialize_selected_columns(...)`.

```cpp
pop.serialize_selected_columns({"status", "agegrp", "variant_hist"}, "population_export", {"some_dir"});
```

That writes all rows `1..pop.popn` to a timestamped CSV file.

If you want serialized output directly to any stream, use the new shared engine:

```cpp
write_agent_pop_data(pop, rows,
                     {"status", "agegrp", "variant_hist"},
                     std::cout,
                     PopOutputLayout::serialized,
                     false,
                     ",",
                     true);
```

Meaning of those args:

- `PopOutputLayout::serialized` = no width padding
- `false` = do not expand multivalues; keep `_hist` as single `"|"`-joined cells
- `","` = column separator
- `true` = CSV-escape cells

So my practical recommendation at the end of simulation is:

- console table summary: `print_agent_pop_table(...)`
- file report table: `print_agent_pop_table(..., ofstream, ...)`
- CSV/data export: `serialize_selected_columns(...)`
- custom serialized stream output: `write_agent_pop_data(..., PopOutputLayout::serialized, ...)`

If you want, I can also suggest a cleaned-up naming scheme now that `write_agent_pop_data(...)` is the real shared engine.