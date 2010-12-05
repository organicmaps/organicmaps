call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE_PLANET% | %INDEXER_TOOL% --generate_intermediate_data=false --generate_final_data=true --use_light_nodes=false --generate_index=true --sort_features --intermediate_data_path=H:\omim\Intermediate\ --output=%2
