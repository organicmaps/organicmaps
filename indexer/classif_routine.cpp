#include "classif_routine.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../indexer/osm2type.hpp"

#include "../coding/reader.hpp"

#include "../std/stdio.hpp"

#include "../base/start_mem_debug.hpp"


namespace classificator
{
  void Read(string const & dir)
  {
    drule::ReadRules((dir + "drawing_rules.bin").c_str());
    if (!classif().ReadClassificator((dir + "classificator.txt").c_str()))
      MYTHROW(Reader::OpenException, ("drawing rules or classificator file"));

    (void)classif().ReadVisibility((dir + "visibility.txt").c_str());
  }

  void parse_osm_types(int start, int end, string const & path)
  {
    for (int i = start; i <= end; ++i)
    {
      char buf[5] = { 0 };
      sprintf(buf, "%d", i);

      string const inFile = path + buf + ".xml";
      ftype::ParseOSMTypes(inFile.c_str(), i);
    }
  }

  void GenerateAndWrite(string const & path)
  {
    // 1. generic types
    parse_osm_types(0, 11, path + "styles/caption-z");
    parse_osm_types(6, 17, path + "styles/osm-map-features-z");

    // 2. POI (not used)
    //parse_osm_types(12, 17, path + "styles/osm-POI-features-z");

    // 3. generate map
    string const inFile = path + "styles/mapswithme.xml";
    for (int i = 0; i <= 17; ++i)
      ftype::ParseOSMTypes(inFile.c_str(), i);

    drule::WriteRules(string(path + "drawing_rules.bin").c_str());
    classif().PrintClassificator(string(path + "classificator.txt").c_str());
  }

  void PrepareForFeatureGeneration()
  {
    classif().SortClassificator();
  }
}
