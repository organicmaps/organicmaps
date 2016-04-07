#include "indexer/classificator_loader.hpp"
#include "indexer/classificator.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "coding/reader_streambuf.hpp"

#include "base/logging.hpp"

#include "std/iostream.hpp"


namespace
{
void ReadCommon(unique_ptr<Reader> classificator,
                unique_ptr<Reader> types)
{
  Classificator & c = classif();
  c.Clear();

  {
    //LOG(LINFO, ("Reading classificator"));
    ReaderStreamBuf buffer(move(classificator));

    istream s(&buffer);
    c.ReadClassificator(s);
  }

  {
    //LOG(LINFO, ("Reading types mapping"));
    ReaderStreamBuf buffer(move(types));

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

    ReadCommon(p.GetReader("classificator.txt"), p.GetReader("types.txt"));

    drule::LoadRules();

    LOG(LDEBUG, ("Reading of classificator finished"));
  }
}
