#include "map_style_reader.hpp"

#include "base/logging.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "std/string.hpp"

namespace
{

char const * const MAP_STYLE_KEY = "MapStyleKeyV1";

const char * const SUFFIX_LEGACY_LIGHT = "";
const char * const SUFFIX_LEGACY_DARK = "_dark";
const char * const SUFFIX_MODERN_CLEAR = "_clear";

string GetStyleSuffix(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleLight:
    return SUFFIX_LEGACY_LIGHT;
   case MapStyleDark:
    return SUFFIX_LEGACY_DARK;
  case MapStyleClear:
    return SUFFIX_MODERN_CLEAR;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return SUFFIX_MODERN_CLEAR;
}

}  // namespace

void StyleReader::SetCurrentStyle(MapStyle mapStyle)
{
  Settings::Set(MAP_STYLE_KEY, static_cast<int>(mapStyle));
}

MapStyle StyleReader::GetCurrentStyle()
{
  int mapStyle;
  if (!Settings::Get(MAP_STYLE_KEY, mapStyle))
    mapStyle = MapStyleClear;
  return static_cast<MapStyle>(mapStyle);
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
