#include "map_style_reader.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

namespace
{
std::string const kSuffixDark = "dark";
std::string const kSuffixLight = "light";
std::string const kSuffixDefaultDark = "_default_dark";
std::string const kSuffixDefaultLight = "_default_light";
std::string const kSuffixVehicleDark = "_vehicle_dark";
std::string const kSuffixVehicleLight = "_vehicle_light";
std::string const kSuffixOutdoorsLight = "_outdoors_light";
std::string const kSuffixOutdoorsDark = "_outdoors_dark";

std::string const kStylesOverrideDir = "styles";

std::string GetStyleRulesSuffix(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleDefaultDark: return kSuffixDefaultDark;
  case MapStyleDefaultLight: return kSuffixDefaultLight;
  case MapStyleVehicleDark: return kSuffixVehicleDark;
  case MapStyleVehicleLight: return kSuffixVehicleLight;
  case MapStyleOutdoorsLight: return kSuffixOutdoorsLight;
  case MapStyleOutdoorsDark: return kSuffixOutdoorsDark;
  case MapStyleMerged: return {};

  case MapStyleCount: break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixDefaultLight;
}

std::string GetStyleResourcesSuffix(MapStyle mapStyle)
{
  // We use the same resources for all light/day and dark/night styles
  // to avoid textures duplication and package size increasing.
  switch (mapStyle)
  {
  case MapStyleDefaultDark:
  case MapStyleVehicleDark:
  case MapStyleOutdoorsDark: return kSuffixDark;
  case MapStyleDefaultLight:
  case MapStyleVehicleLight:
  case MapStyleOutdoorsLight: return kSuffixLight;
  case MapStyleMerged: return {};

  case MapStyleCount: break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixLight;
}
}  // namespace

StyleReader::StyleReader() : m_mapStyle(kDefaultMapStyle) {}

void StyleReader::SetCurrentStyle(MapStyle mapStyle)
{
  m_mapStyle = mapStyle;
}

MapStyle StyleReader::GetCurrentStyle() const
{
  return m_mapStyle;
}

bool StyleReader::IsCarNavigationStyle() const
{
  return m_mapStyle == MapStyle::MapStyleVehicleLight || m_mapStyle == MapStyle::MapStyleVehicleDark;
}

ReaderPtr<Reader> StyleReader::GetDrawingRulesReader() const
{
  std::string rulesFile = std::string("drules_proto") + GetStyleRulesSuffix(GetCurrentStyle()) + ".bin";

#ifndef BUILD_DESIGNER
  // The Designer skips the styles/ override: Build Style writes fresh files
  // into the writable dir, which the default "wrf" scope already finds first,
  // and a stale override would shadow every rebuild.
  auto overriddenRulesFile = base::JoinPath(GetPlatform().WritableDir(), kStylesOverrideDir, rulesFile);
  if (Platform::IsFileExistsByFullPath(overriddenRulesFile))
    rulesFile = overriddenRulesFile;
#endif
  return GetPlatform().GetReader(rulesFile);
}

ReaderPtr<Reader> StyleReader::GetResourceReader(std::string const & file, std::string_view density) const
{
  std::string resFile =
      base::JoinPath("symbols", std::string{density}, GetStyleResourcesSuffix(GetCurrentStyle()), file);

#ifndef BUILD_DESIGNER
  // See note in GetDrawingRulesReader().
  auto overriddenResFile = base::JoinPath(GetPlatform().WritableDir(), kStylesOverrideDir, resFile);
  if (GetPlatform().IsFileExistsByFullPath(overriddenResFile))
    resFile = overriddenResFile;
#endif
  return GetPlatform().GetReader(resFile);
}

ReaderPtr<Reader> StyleReader::GetDefaultResourceReader(std::string const & file) const
{
  return GetPlatform().GetReader(base::JoinPath("symbols/default", file));
}

StyleReader & GetStyleReader()
{
  static StyleReader instance;
  return instance;
}
