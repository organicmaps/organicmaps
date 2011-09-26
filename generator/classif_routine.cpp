#include "classif_routine.hpp"
#include "osm2type.hpp"

#include "../indexer/classificator.hpp"
#include "../indexer/drawing_rules.hpp"
#include "../indexer/file_reader_stream.hpp"
#include "../indexer/scales.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../base/start_mem_debug.hpp"


namespace classificator
{
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
    // Experimental - add drawing rules in program.

//    // 1. Read rules.
//    string const name = "drawing_rules.bin";
//    ReaderPtrStream rulesS(GetPlatform().GetReader(name));
//    drule::ReadRules(rulesS);

//    // 2. Append rules.
//    //int const color = 0;
//    int const color = 0xB5D6F1;
//    //double const pixWidth = 1.5;
//    for (int i = 0; i <= scales::GetUpperScale(); ++i)
//    {
//      //size_t const ind = drule::rules().AddLineRule(i, color, pixWidth);
//      size_t const ind = drule::rules().AddAreaRule(i, color);
//      LOG_SHORT(LINFO, ("Scale = ", i, "; Index = ", ind));
//    }

//    drule::WriteRules((path + name).c_str());
//    return;

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
