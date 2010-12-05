call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE% | %INDEXER_TOOL% --generate_intermediate_data=true --generate_final_data=false --use_light_nodes=true --generate_index=false --intermediate_data_path=D:\Temp\
%UNZIP_TOOL% -dc %OSM_FILE% | %INDEXER_TOOL% --generate_intermediate_data=false --generate_final_data=true --use_light_nodes=true --generate_index=true --sort_features --intermediate_data_path=D:\Temp\ --output=%2 --bucketing_level=0
