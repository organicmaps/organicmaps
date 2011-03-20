call set_vars.bat %1 %2

%GENERATOR_TOOL% --generate_geometry=true --output=%2 --bucketing_level=0
