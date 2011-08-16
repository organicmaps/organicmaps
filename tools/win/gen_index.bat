call set_vars.bat %1 %2

%GENERATOR_TOOL% --generate_index=true --generate_search_index=true --intermediate_data_path=D:\Temp\ --output=%2 --bucketing_level=0
