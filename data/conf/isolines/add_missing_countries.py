import json
from os import listdir

data = object()

with open('countries-to-generate.json') as f1:
  countries = set()

  data = json.load(f1)
  tree = data['countryParams']
  for e in tree:
    countries.add(e['key'])

  for f2 in listdir('../../borders/'):
    c = f2[:-5]
    if c not in countries:
      print(c)

      entry = {
            "key": c,
            "value": {
                "profileName": "normal",
                "tileCoordsSubset": list(),
                "tilesAreBanned": False
            }
        }

      tree.append(entry)

with open('countries-to-generate.json', "w") as f3:
  json.dump(data, f3, indent=4)
