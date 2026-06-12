#include "map_style_reader.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

namespace
{
std::string const kSuffixDark = "dark";
std::string const kSuffixLight = "light";

std::string const kStylesOverrideDir = "styles";

#ifdef BUILD_DESIGNER
std::string const kDesignerRulesFile = "drules_design.bin";
#endif  // BUILD_DESIGNER

// Light and dark of a style share one family file; the variant is selected at load time.
std::string GetStyleRulesFamily(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleDefaultDark:
  case MapStyleDefaultLight: return "default";
  case MapStyleVehicleDark:
  case MapStyleVehicleLight: return "vehicle";
  case MapStyleOutdoorsLight:
  case MapStyleOutdoorsDark: return "outdoors";
  case MapStyleMerged: return "merged";

  case MapStyleCount: break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return "default";
}

std::string GetDrawingRulesFile(MapStyle mapStyle)
{
#ifdef BUILD_DESIGNER
  (void)mapStyle;
  return kDesignerRulesFile;
#else
  return "drules_" + GetStyleRulesFamily(mapStyle) + ".bin";
#endif  // BUILD_DESIGNER
}

std::string GetStyleResourcesSuffix(MapStyle mapStyle)
{
#ifdef BUILD_DESIGNER
  return kSuffixDesignTool;
#else
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
#endif  // BUILD_DESIGNER
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

ReaderPtr<Reader> StyleReader::GetDrawingRulesReader(MapStyle mapStyle) const
{
  std::string const rulesFile = GetDrawingRulesFile(mapStyle);
#ifdef BUILD_DESIGNER
  // For Designer tool we have to look first into the resource folder.
  return GetPlatform().GetReader(rulesFile, "rwf");
#else
  return GetPlatform().GetReader(rulesFile);
#endif
}

bool StyleReader::ReadDrawingRulesOverride(MapStyle mapStyle, std::string & buffer) const
{
  auto const path = base::JoinPath(GetPlatform().WritableDir(), kStylesOverrideDir, GetDrawingRulesFile(mapStyle));
  if (!Platform::IsFileExistsByFullPath(path))
    return false;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(path)).ReadAsString(buffer);
    return true;
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Failed to read drules override", path, e.Msg()));
    return false;
  }
}

size_t StyleReader::GetDrawingRulesVariant(MapStyle mapStyle) const
{
#ifdef BUILD_DESIGNER
  // The designer ships a single-variant file (drules_design.bin), so every style maps to index 0.
  (void)mapStyle;
  return 0;
#else
  // Family files store the light variant at index 0 and dark at index 1 (see merge_variants.py);
  // the single-variant merged file only has index 0.
  return (mapStyle != MapStyleMerged && MapStyleIsDark(mapStyle)) ? 1 : 0;
#endif
}

ReaderPtr<Reader> StyleReader::GetResourceReader(std::string const & file, std::string_view density) const
{
  std::string resFile =
      base::JoinPath("symbols", std::string{density}, GetStyleResourcesSuffix(GetCurrentStyle()), file);

  auto overriddenResFile = base::JoinPath(GetPlatform().WritableDir(), kStylesOverrideDir, resFile);
  if (GetPlatform().IsFileExistsByFullPath(overriddenResFile))
    resFile = overriddenResFile;

#ifdef BUILD_DESIGNER
  // For Designer tool we have to look first into the resource folder.
  return GetPlatform().GetReader(resFile, "rwf");
#else
  return GetPlatform().GetReader(resFile);
#endif
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
