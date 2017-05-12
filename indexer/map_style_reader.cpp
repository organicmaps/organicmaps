#include "map_style_reader.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

namespace
{
std::string const kSuffixDark = "_dark";
std::string const kSuffixClear = "_clear";
std::string const kSuffixVehicleDark = "_vehicle_dark";
std::string const kSuffixVehicleClear = "_vehicle_clear";

std::string const kStylesOverrideDir = "styles";

std::string GetStyleRulesSuffix(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleDark:
    return kSuffixDark;
  case MapStyleClear:
    return kSuffixClear;
  case MapStyleVehicleDark:
    return kSuffixVehicleDark;
  case MapStyleVehicleClear:
    return kSuffixVehicleClear;
  case MapStyleMerged:
    return std::string();

  case MapStyleCount:
    break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixClear;
}

std::string GetStyleResourcesSuffix(MapStyle mapStyle)
{
  // We use the same resources for default and vehicle styles
  // to avoid textures duplication and package size increasing.
  switch (mapStyle)
  {
  case MapStyleDark:
  case MapStyleVehicleDark:
    return kSuffixDark;
  case MapStyleClear:
  case MapStyleVehicleClear:
    return kSuffixClear;
  case MapStyleMerged:
    return std::string();

  case MapStyleCount:
    break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixClear;
}
}  // namespace

StyleReader::StyleReader()
  : m_mapStyle(kDefaultMapStyle)
{}

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
  std::string rulesFile =
      std::string("drules_proto") + GetStyleRulesSuffix(GetCurrentStyle()) + ".bin";

  auto overriddenRulesFile =
      my::JoinFoldersToPath({GetPlatform().WritableDir(), kStylesOverrideDir}, rulesFile);
  if (GetPlatform().IsFileExistsByFullPath(overriddenRulesFile))
    rulesFile = overriddenRulesFile;

  return GetPlatform().GetReader(rulesFile);
}

ReaderPtr<Reader> StyleReader::GetResourceReader(std::string const & file,
                                                 std::string const & density)
{
  std::string const resourceDir =
      std::string("resources-") + density + GetStyleResourcesSuffix(GetCurrentStyle());
  std::string resFile = my::JoinFoldersToPath(resourceDir, file);

  auto overriddenResFile =
      my::JoinFoldersToPath({GetPlatform().WritableDir(), kStylesOverrideDir}, resFile);
  if (GetPlatform().IsFileExistsByFullPath(overriddenResFile))
    resFile = overriddenResFile;

  return GetPlatform().GetReader(resFile);
}

ReaderPtr<Reader> StyleReader::GetDefaultResourceReader(std::string const & file)
{
  return GetPlatform().GetReader(my::JoinFoldersToPath("resources-default", file));
}

StyleReader & GetStyleReader()
{
  static StyleReader instance;
  return instance;
}
