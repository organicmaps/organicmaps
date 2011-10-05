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

  /*
  class DoFindSymbol
  {
    string m_name;
  public:
    DoFindSymbol(string const & name) : m_name(name) {}

    void operator() (int s, int t, int i, drule::BaseRule const * p)
    {
      if (s == 17 && t == drule::symbol)
      {
        string name;
        p->GetSymbol(name);
        if (name == m_name)
        {
          LOG(LINFO, ("Found rule with index = ", i));
        }
      }
    }
  };
  */

  void GenerateAndWrite(string const & path)
  {
    using namespace drule;

    // Experimental - add drawing rules in program.

//    // 1. Read rules.
//    string const name = "drawing_rules.bin";
//    ReaderPtrStream rulesS(GetPlatform().GetReader(name));
//    ReadRules(rulesS);

//    // 2. Find spesial rule.
//    //rules().ForEachRule(DoFindSymbol("supermarket"));

//    // 3. Append rules.
//    //int const color = 0;
//    int const color = 0xB5D6F1;
//    //double const pixWidth = 1.5;
//    for (int i = 0; i <= scales::GetUpperScale(); ++i)
//    {
//      //size_t const ind = rules().AddLineRule(i, color, pixWidth);
//      size_t const ind = rules().AddAreaRule(i, color);
//      LOG_SHORT(LINFO, ("Scale = ", i, "; Index = ", ind));
//    }
//    //size_t const ind = rules().AddSymbolRule(17, "supermarket");
//    //LOG_SHORT(LINFO, ("Index = ", ind));

//    // 4. Write rules.
//    WriteRules((path + name).c_str());
//    // 5. Exit.
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
