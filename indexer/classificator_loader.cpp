#include "classificator_loader.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../platform/platform.hpp"

#include "../coding/reader_streambuf.hpp"

#include "../base/logging.hpp"

#include "../std/iostream.hpp"


namespace
{
  void ReadCommon(Reader * classificator,
                  Reader * types)
  {
    Classificator & c = classif();
    c.Clear();

    {
      //LOG(LINFO, ("Reading classificator"));
      ReaderStreamBuf buffer(classificator);

      istream s(&buffer);
      c.ReadClassificator(s);
    }

    {
      //LOG(LINFO, ("Reading types mapping"));
      ReaderStreamBuf buffer(types);

      istream s(&buffer);
      c.ReadTypesMapping(s);
    }
  }
}

namespace classificator
{
  void Load()
  {
    LOG(LDEBUG, ("Reading of classificator started"));

    Platform & p = GetPlatform();

    ReadCommon(p.GetReader("classificator.txt"),            
               p.GetReader("types.txt"));

    drule::LoadRules();

    LOG(LDEBUG, ("Reading of classificator finished"));
  }
}
