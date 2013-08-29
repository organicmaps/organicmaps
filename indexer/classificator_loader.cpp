#include "classificator_loader.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"

#include "../coding/reader_streambuf.hpp"

#include "../base/logging.hpp"

#include "../std/iostream.hpp"


namespace classificator
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

  void Load()
  {
    LOG(LDEBUG, ("Reading of classificator started"));

    Platform & p = GetPlatform();

    ReadCommon(p.GetReader("classificator.txt"),            
               p.GetReader("types.txt"));

    //LOG(LINFO, ("Reading of drawing rules"));
    drule::RulesHolder & rules = drule::rules();

#if defined(OMIM_PRODUCTION)
    // Load from proto buffer binary file.
    string buffer;
    ModelReaderPtr(p.GetReader(DRAWING_RULES_BIN_FILE)).ReadAsString(buffer);

    rules.LoadFromBinaryProto(buffer);
#else
    // Load from proto buffer text file.
    string buffer;
    ModelReaderPtr(p.GetReader(DRAWING_RULES_TXT_FILE)).ReadAsString(buffer);

    rules.LoadFromTextProto(buffer);
#endif

    LOG(LDEBUG, ("Reading of classificator finished"));
  }
}
