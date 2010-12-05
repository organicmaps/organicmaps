call set_vars.bat %1 %2

%UNZIP_TOOL% -dc %OSM_FILE_PLANET% | %INDEXER_TOOL% --generate_intermediate_data=true --generate_final_data=false --use_light_nodes=false --generate_index=false --intermediate_data_path=H:\omim\Intermediate\
