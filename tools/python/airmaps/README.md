# airmaps - building of maps using airflow.

## Storage

Repository of result and temporary files.
Currently, the storage is a webdav server.

## Description of DAGs:

1. Update_planet - updates .o5m planet file.

2. Build_coastline - builds coastline files.

3. Generate_open_source_maps - builds free maps for maps.me

All results will be published on the storage.
