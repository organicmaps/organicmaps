call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE% | %GENERATOR_TOOL% --make_coasts --use_light_nodes --intermediate_data_path=D:\Temp\ 
%UNZIP_TOOL% -dc %OSM_FILE% | %GENERATOR_TOOL% --emit_coasts --generate_features --generate_geometry --generate_index --split_by_polygons --generate_world --use_light_nodes --intermediate_data_path=D:\Temp\ 
