# Creates file with all OSM unicode chars inside.
# This file can be used as input for BMFont program
# to select symbols used in OSM.

import codecs
    
f = open("results.txt", "r")
lines = f.readlines()
f.close()

# clear some garbage from results
lines.remove("Code\tCount\t#\tSymbol\n")
lines.remove("=========================================================\n")
lines.pop()
lines.pop()


f = codecs.open('results.unicode', 'w', 'utf-16')
for line in lines:
  f.write( unichr(int(line[0:line.find("\t")])) )
