#include "map_style_reader.hpp"

#include "base/logging.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "std/string.hpp"

namespace
{

char const * const kMapStyleKey = "MapStyleKeyV1";

const char * const kSuffixLegacyLight = "";
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
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixModernClear;
}

}  // namespace

void StyleReader::SetCurrentStyle(MapStyle mapStyle)
{
  Settings::Set(kMapStyleKey, static_cast<int>(mapStyle));
}

MapStyle StyleReader::GetCurrentStyle()
{
  int mapStyle = MapStyleLight;
// @TODO(shalnev) It's a hotfix to fix tests generator_tests and map_tests.
// Tests should work with any styles.
#if defined(OMIM_OS_ANDROID) || defined(OMIM_OS_IPHONE)
  if (!Settings::Get(kMapStyleKey, mapStyle))
    mapStyle = MapStyleClear;
#endif

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
