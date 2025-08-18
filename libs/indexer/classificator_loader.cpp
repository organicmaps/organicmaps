#include "indexer/classificator_loader.hpp"
#include "indexer/classificator.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/logging.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace
{
void ReadCommon(std::unique_ptr<Reader> classificator, std::unique_ptr<Reader> types)
{
  Classificator & c = classif();
  c.Clear();

  {
    // LOG(LINFO, ("Reading classificator"));
    ReaderStreamBuf buffer(std::move(classificator));

    std::istream s(&buffer);
    c.ReadClassificator(s);
  }

  {
    // LOG(LINFO, ("Reading types mapping"));
    ReaderStreamBuf buffer(std::move(types));

    std::istream s(&buffer);
    c.ReadTypesMapping(s);
  }
}
}  // namespace

namespace classificator
{
void Load()
{
  LOG(LDEBUG, ("Reading of classificator started"));

  Platform & p = GetPlatform();

  MapStyle const originMapStyle = GetStyleReader().GetCurrentStyle();

  for (size_t i = 0; i < MapStyleCount; ++i)
  {
    auto const mapStyle = static_cast<MapStyle>(i);
    // Read the merged style only if it was requested.
    if (mapStyle != MapStyleMerged || originMapStyle == MapStyleMerged)
    {
      GetStyleReader().SetCurrentStyle(mapStyle);
      ReadCommon(p.GetReader("classificator.txt"), p.GetReader("types.txt"));

      drule::LoadRules();
    }
  }

  GetStyleReader().SetCurrentStyle(originMapStyle);

  LOG(LDEBUG, ("Reading of classificator finished"));
}

void LoadTypes(std::string const & classificatorFileStr, std::string const & typesFileStr)
{
  ReadCommon(std::make_unique<MemReaderWithExceptions>(classificatorFileStr.data(), classificatorFileStr.size()),
             std::make_unique<MemReaderWithExceptions>(typesFileStr.data(), typesFileStr.size()));
}
}  // namespace classificator
