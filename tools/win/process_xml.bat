call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE% | %GENERATOR_TOOL% --preprocess_xml=true --use_light_nodes=true --intermediate_data_path=D:\Temp\
