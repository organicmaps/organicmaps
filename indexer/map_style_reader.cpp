#include "map_style_reader.hpp"

#include "base/logging.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

#include "std/string.hpp"

namespace
{

const char * const kSuffixDark = "_dark";
const char * const kSuffixClear = "_clear";

string GetStyleSuffix(MapStyle mapStyle)
{
  switch (mapStyle)
  {
   case MapStyleDark:
    return kSuffixDark;
  case MapStyleClear:
    return kSuffixClear;
  case MapStyleMerged:
    return string();

  case MapStyleCount:
    break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixClear;
}

}  // namespace

StyleReader::StyleReader()
  : m_mapStyle(kDefaultMapStyle)
{
}

void StyleReader::SetCurrentStyle(MapStyle mapStyle)
{
  m_mapStyle = mapStyle;
}

MapStyle StyleReader::GetCurrentStyle()
{
  return m_mapStyle;
}

ReaderPtr<Reader> StyleReader::GetDrawingRulesReader()
{
  string const rulesFile = string("drules_proto") + GetStyleSuffix(GetCurrentStyle()) + ".bin";
  return GetPlatform().GetReader(rulesFile);
}

ReaderPtr<Reader> StyleReader::GetResourceReader(string const & file, string const & density)
{
  string const resourceDir = string("resources-") + density + GetStyleSuffix(GetCurrentStyle());
  return GetPlatform().GetReader(my::JoinFoldersToPath(resourceDir, file));
}

StyleReader & GetStyleReader()
{
  static StyleReader instance;
  return instance;
}
