#include "map_style_reader.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

namespace
{
std::string const kSuffixDark = "dark";
std::string const kSuffixLight = "light";

std::string const kStylesOverrideDir = "styles";

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
  return "drules_" + GetStyleRulesFamily(mapStyle) + ".bin";
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

ReaderPtr<Reader> StyleReader::GetDrawingRulesReader(MapStyle mapStyle) const
{
  // The default scope searches the writable dir before the app resources, so the Designer's
  // freshly rebuilt family file (and a non-designer styles/ override applied separately) shadows
  // the bundled one without touching the bundle.
  return GetPlatform().GetReader(GetDrawingRulesFile(mapStyle));
}

bool StyleReader::ReadDrawingRulesOverride(MapStyle mapStyle, std::string & buffer) const
{
  // The Designer rebuilds the bundled family file in the writable dir, which GetDrawingRulesReader
  // already finds first; a stale styles/ override would shadow every rebuild, so skip it.
  if (m_designerMode)
    return false;

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
  // Family files store the light variant at index 0 and dark at index 1 (see merge_variants.py);
  // the single-variant merged file only has index 0.
  return (mapStyle != MapStyleMerged && MapStyleIsDark(mapStyle)) ? 1 : 0;
}

ReaderPtr<Reader> StyleReader::GetResourceReader(std::string const & file, std::string_view density) const
{
  std::string resFile =
      base::JoinPath("symbols", std::string{density}, GetStyleResourcesSuffix(GetCurrentStyle()), file);

  // See note in ReadDrawingRulesOverride(): the Designer rebuilds symbols in the writable dir
  // directly, so skip the styles/ override that would otherwise shadow them.
  if (!m_designerMode)
  {
    auto overriddenResFile = base::JoinPath(GetPlatform().WritableDir(), kStylesOverrideDir, resFile);
    if (GetPlatform().IsFileExistsByFullPath(overriddenResFile))
      resFile = overriddenResFile;
  }
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
