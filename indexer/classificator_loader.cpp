#include "classificator_loader.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"

#include "../coding/reader_streambuf.hpp"

#include "../base/logging.hpp"

#include "../std/fstream.hpp"


namespace classificator
{
  void ReadCommon(Reader * classificator,
                  Reader * visibility,
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
      //LOG(LINFO, ("Reading visibility"));
      ReaderStreamBuf buffer(visibility);

      istream s(&buffer);
      c.ReadVisibility(s);
    }

    {
      //LOG(LINFO, ("Reading types mapping"));
      ReaderStreamBuf buffer(types);

      istream s(&buffer);
      c.ReadTypesMapping(s);
    }
  }

  void ReadVisibility(string const & fPath)
  {
    ifstream s(fPath.c_str());
    classif().ReadVisibility(s);
  }

  void Load()
  {
    LOG(LDEBUG, ("Reading of classificator started"));

    Platform & p = GetPlatform();

    ReadCommon(p.GetReader("classificator.txt"),
               p.GetReader("visibility.txt"),
               p.GetReader("types.txt"));

    //LOG(LINFO, ("Reading of drawing rules"));
    drule::RulesHolder & rules = drule::rules();

#if defined(OMIM_PRODUCTION) || defined(USE_BINARY_STYLES)
    // Load from proto buffer binary file.
    ReaderStreamBuf buffer(p.GetReader(DRAWING_RULES_BIN_FILE));

    istream s(&buffer);
    rules.LoadFromBinaryProto(s);
#else
    // Load from proto buffer text file.
    string buffer;
    ModelReaderPtr(p.GetReader(DRAWING_RULES_TXT_FILE)).ReadAsString(buffer);

    rules.LoadFromTextProto(buffer);
#endif

    LOG(LDEBUG, ("Reading of classificator finished"));
  }
}
