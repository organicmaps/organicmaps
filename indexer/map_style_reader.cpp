#include "map_style_reader.hpp"

#include "base/logging.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

#include "std/string.hpp"

namespace
{

const char * const kSuffixLegacyLight = "_legacy";
const char * const kSuffixLegacyDark = "_dark";
const char * const kSuffixModernClear = "_clear";

string GetStyleSuffix(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleLight:
    return kSuffixLegacyLight;
   case MapStyleDark:
    return kSuffixLegacyDark;
  case MapStyleClear:
    return kSuffixModernClear;
  case MapStyleMerged:
    return string();

  case MapStyleCount:
    break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixModernClear;
}

}  // namespace

StyleReader::StyleReader()
  : m_mapStyle(MapStyleLight)
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
