call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE% | %GENERATOR_TOOL% --generate_features=true --use_light_nodes=true --split_by_polygons=true --generate_world=true --intermediate_data_path=D:\Temp\
