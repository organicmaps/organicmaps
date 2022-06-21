#include "map_style_reader.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

namespace
{
std::string const kSuffixDark = "_dark";
std::string const kSuffixClear = "_clear";
std::string const kSuffixVehicleDark = "_vehicle_dark";
std::string const kSuffixVehicleClear = "_vehicle_clear";

std::string const kStylesOverrideDir = "styles";

#ifdef BUILD_DESIGNER
std::string const kSuffixDesignTool = "_design";
#endif // BUILD_DESIGNER

std::string GetStyleRulesSuffix(MapStyle mapStyle)
{
#ifdef BUILD_DESIGNER
  return kSuffixDesignTool;
#else
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
#endif // BUILD_DESIGNER
}

std::string GetStyleResourcesSuffix(MapStyle mapStyle)
{
#ifdef BUILD_DESIGNER
  return kSuffixDesignTool;
#else
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
#endif // BUILD_DESIGNER
}
}  // namespace

StyleReader::StyleReader()
  : m_mapStyle(kDefaultMapStyle)
{}

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
  return m_mapStyle == MapStyle::MapStyleVehicleClear ||
         m_mapStyle == MapStyle::MapStyleVehicleDark;
}

ReaderPtr<Reader> StyleReader::GetDrawingRulesReader()
{
  std::string rulesFile =
      std::string("drules_proto") + GetStyleRulesSuffix(GetCurrentStyle()) + ".bin";

  Platform const & pl = GetPlatform();
  if (m_isStylesOverrideEnabled)
  {
    auto overriddenRulesFile =
        base::JoinPath(pl.WritableDir(), kStylesOverrideDir, rulesFile);
    if (pl.IsFileExistsByFullPath(overriddenRulesFile))
    {
      rulesFile = overriddenRulesFile;
      m_isVisibilityOverrideEnabled = true;
    }
  }

#ifdef BUILD_DESIGNER
  // For Designer tool we have to look first into the resource folder.
  return pl.GetReader(rulesFile, "rwf");
#else
  return pl.GetReader(rulesFile);
#endif
}

ReaderPtr<Reader> StyleReader::GetResourceReader(std::string const & file,
                                                 std::string const & density) const
{
  std::string const resourceDir =
      std::string("resources-") + density + GetStyleResourcesSuffix(GetCurrentStyle());
  std::string resFile = base::JoinPath(resourceDir, file);

  Platform const & pl = GetPlatform();
  if (m_isStylesOverrideEnabled)
  {
    auto overriddenResFile = base::JoinPath(pl.WritableDir(), kStylesOverrideDir, resFile);
    if (pl.IsFileExistsByFullPath(overriddenResFile))
      resFile = overriddenResFile;
  }

#ifdef BUILD_DESIGNER
  // For Designer tool we have to look first into the resource folder.
  return pl.GetReader(resFile, "rwf");
#else
  return pl.GetReader(resFile);
#endif
}

ReaderPtr<Reader> StyleReader::GetDefaultResourceReader(std::string const & file) const
{
  return GetPlatform().GetReader(base::JoinPath("resources-default", file));
}

StyleReader & GetStyleReader()
{
  static StyleReader instance;
  return instance;
}
