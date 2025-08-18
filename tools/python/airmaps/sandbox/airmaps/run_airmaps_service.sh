#!/usr/bin/env bash

export PYTHONPATH=/omim/tools/python
export AIRFLOW_HOME=/airflow_home

# Initialize the database.
airflow initdb

# Start the web server, default port is 8880.
airflow webserver -p 8880 &

# Start the scheduler.
airflow scheduler
