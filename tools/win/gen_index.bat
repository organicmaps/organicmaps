call set_vars.bat %1 %2

%INDEXER_TOOL% --generate_index=true --intermediate_data_path=D:\Temp\ --output=%2 --bucketing_level=0
