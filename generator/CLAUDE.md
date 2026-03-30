# Generator Agent Instructions

The generator converts OpenStreetMap (OSM) data into `.mwm` binary map files used by Organic Maps.

## Build
```bash
cmake --build build-$YOUR_NAME --target generator_tool
```

## Pipeline stages
OSM data flows through these stages:

1. **Collectors** (`collector_*.cpp`) -- gather auxiliary data (cameras, building parts, road access, etc.)
2. **Translators** (`translator.cpp`) -- convert `OsmElement` into intermediate features via type mapping
3. **Processors** (`processor_*.cpp`) -- handle coastlines, countries, complex features
4. **Final processors** (`final_processor_*.cpp`) -- produce output for cities, coastlines, countries, world map
5. **Index generation** -- builds search, routing, and geometry indices

## Key classes
- `OsmElement` -- raw OSM node/way/relation
- `FeatureBuilder` (`feature_builder.hpp`) -- intermediate feature representation (NOT `FeatureType`, which is in `indexer/` for reading)
- `Translator` -- applies type mapping from `data/mapcss-mapping.csv` to `OsmElement`
- `GeneratorTool` (`generator_tool/generator_tool.cpp`) -- CLI entry point with flag-based stage selection

## Testing
- Tests: `generator_tests/` (unit), `generator_integration_tests/` (integration)
- Most-edited test file: `generator_tests/osm_type_test.cpp` -- add tests here when adding new OSM type mappings
- Run: `ctest -j --test-dir build-$YOUR_NAME -R generator_tests --stop-on-failure --output-on-failure`

## Python maps_generator
`tools/python/maps_generator/` -- CLI for orchestrating full map builds:
```bash
python -m maps_generator --countries="CountryName"
```

## Related data files
- `data/mapcss-mapping.csv` -- OSM tag -> OM type mapping (input to translator)
- `data/classificator.txt` -- auto-generated type hierarchy (do NOT edit)
- `data/replaced_tags.txt` -- merges similar OSM tags before translation
- `data/mixed_tags.txt` -- pedestrian streets of high popularity
