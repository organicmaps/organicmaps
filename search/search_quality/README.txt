This document describes how to use the tools for search quality analysis.


1. Prerequisites.

   * Get the latest version of the project (https://codeberg.org/comaps/comaps) and build it.

   * Get the latest samples.lisp file with search queries. If you don't know
     how to get it, please, contact the search team.

   * Install Common Lisp. Note that there are many implementations,
     but we recommend to use SBCL (http://www.sbcl.org/).

   * Install Python 3.x and packages for data analysis (sklearn, scipy, numpy, pandas, matplotlib).

   * Download maps necessary for search quality tests.
     For example:

     ./download-maps.sh -v 160524

     will download all necessary maps of version 160524 to the current directory.


2. This section describes how to run search engine on a set of search
   queries and how to get a CSV file with search engine output.

    i) Run gen-samples.lisp script to get search queries with lists of
       vital or relevant responses in JSON format. For example:

       ./gen-samples.lisp < samples.lisp > samples.jsonl

   ii) Run features_collector_tool from the build directory.
       For example:

       features_collector_tool --mwm_path path-to-downloaded-maps \
         --json_in samples.jsonl \
         --stats_path /tmp/stats.txt \
         2>/dev/null >samples.csv

       runs search engine on all queries from samples.jsonl, prints
       useful info to /tmp/stats.txt and generates a CSV file with
       search engine output on each query.

       The resulting CSV file is ready for analysis, i.e. for search
       quality evaluation, ranking models learning etc. For details,
       take a look at scoring_model.py script.

  iii) To take a quick look at what the search returns without
       launching the application, consider using search_quality_tool:

       search_quality_tool --viewport=moscow \
         --queries_path=path-to-omim/search/search_quality/search_quality_tool/queries.txt
         --top 1 \
         2>/dev/null

       By default, map files in path-to-omim/data are used.
