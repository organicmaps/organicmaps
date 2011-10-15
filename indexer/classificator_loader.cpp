#include "classificator_loader.hpp"
#include "file_reader_stream.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../coding/file_reader.hpp"

#include "../base/logging.hpp"


namespace classificator
{
  void Read(ReaderType const & rules,
            ReaderType const & classificator,
            ReaderType const & visibility,
            ReaderType const & types)
  {
//    LOG(LINFO, ("Reading drawing rules"));
    ReaderPtrStream rulesS(rules);
    drule::ReadRules(rulesS);

    string buffer;

    Classificator & c = classif();
    c.Clear();

//    LOG(LINFO, ("Reading classificator"));
    classificator.ReadAsString(buffer);
    c.ReadClassificator(buffer);

//    LOG(LINFO, ("Reading visibility"));
    visibility.ReadAsString(buffer);
    c.ReadVisibility(buffer);

//    LOG(LINFO, ("Reading types mapping"));
    types.ReadAsString(buffer);
    c.ReadTypesMapping(buffer);

//    LOG(LINFO, ("Reading of classificator done"));
  }

  void ReadVisibility(string const & fPath)
  {
    string buffer;
    ReaderType(new FileReader(fPath)).ReadAsString(buffer);
    classif().ReadVisibility(buffer);
  }

  uint32_t GetTestDefaultType()
  {
    vector<string> v;
    v.push_back("highway");
    v.push_back("motorway");
    return classif().GetTypeByPath(v);
  }
}
