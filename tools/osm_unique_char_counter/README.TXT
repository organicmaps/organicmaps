This console app used to calculate total number of unique unicode characters in planet.osm XML data.
It prints current statistics and writes file with all statistics at the end.

These tags are analyzed:

<tag k="name[optional]" v="TextToCountChars"/>

Usage on Windows:
# bzip2 -dc planet.bz2 | osm_unique_char_counter.exe