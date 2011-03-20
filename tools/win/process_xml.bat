call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE% | %GENERATOR_TOOL% --preprocess_xml=true --generate_features=false --generate_geometry=false --use_light_nodes=true --generate_index=false --intermediate_data_path=D:\Temp\
