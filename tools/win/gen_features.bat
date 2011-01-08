call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE% | %INDEXER_TOOL% --generate_features=true --use_light_nodes=true --intermediate_data_path=D:\Temp\ --output=%2 --bucketing_level=0
