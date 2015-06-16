#include "map_style_reader.hpp"

#include "base/logging.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "std/string.hpp"

namespace
{

char const * const MAP_STYLE_KEY = "MapStyleKey";

string GetDrawingRulesFile(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleLight:
    return "drules_proto.bin";
  case MapStyleDark:
    return "drules_proto_dark.bin";
  default:
    LOG(LWARNING, ("Unknown map style", mapStyle));
    return string();
  }
}

string GetStyleSuffix(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleLight:
    return "";
   case MapStyleDark:
    return "_dark";
  default:
    LOG(LWARNING, ("Unknown map style", mapStyle));
    return string();
  }
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
    mapStyle = MapStyleLight;
  return static_cast<MapStyle>(mapStyle);
}

ReaderPtr<Reader> StyleReader::GetDrawingRulesReader()
{
  string const rulesFile = GetDrawingRulesFile(GetCurrentStyle());
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
