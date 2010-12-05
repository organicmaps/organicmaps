set INDEXER_TOOL="..\..\out\%1\indexer_tool.exe"

%INDEXER_TOOL% --generate_intermediate_data=true --generate_final_data=false --use_light_nodes=true --generate_index=false --intermediate_data_path=D:\Temp\ < ..\..\data\tests\%2.osm
%INDEXER_TOOL% --generate_intermediate_data=false --generate_final_data=true --use_light_nodes=true --generate_index=true --sort_features --intermediate_data_path=D:\Temp\ --output=%2 < ..\..\data\tests\%2.osm  --bucketing_level=0
