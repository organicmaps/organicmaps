#include "map_style_manager.hpp"

#include "base/file_name_utils.hpp"
#include "platform/platform.hpp"

#include <glaze/json.hpp>

namespace
{
std::string constexpr kStylesDir = "styles";
std::string constexpr kStyleFileName = "style.json";

MapStyleName constexpr kDefaultStyle = "default";
MapStyleName constexpr kVehicleStyle = "vehicle";
MapStyleName constexpr kOutdoorsStyle = "outdoors";
MapStyleName constexpr kMergedStyle = "merged";
}  // namespace

MapStyleManager & MapStyleManager::Instance()
{
  static MapStyleManager instance;
  return instance;
}

void MapStyleManager::SetStyle(MapStyleName name)
{
  m_currentStyle.store(&GetStyle(name));
}

void MapStyleManager::SetTheme(MapStyleTheme const theme)
{
  m_currentTheme.store(theme);
}

MapStyleName MapStyleManager::GetCurrentStyleName() const
{
  return m_currentStyle.load()->name;
}

std::vector<MapStyleName> MapStyleManager::GetAvailableStyles() const
{
  return m_styles | std::views::keys | std::ranges::to<std::vector>();
}

MapStyleTheme MapStyleManager::GetCurrentTheme() const
{
  return m_currentTheme;
}

MapStyleName MapStyleManager::GetDefaultStyleName()
{
  return kDefaultStyle;
}

MapStyleName MapStyleManager::GetVehicleStyleName()
{
  return kVehicleStyle;
}

MapStyleName MapStyleManager::GetOutdoorsStyleName()
{
  return kOutdoorsStyle;
}

MapStyleName MapStyleManager::GetMergedStyleName()
{
  return kMergedStyle;
}

bool MapStyleManager::IsValidStyle(MapStyleName styleName) const
{
  return m_styles.contains(styleName);
}

bool MapStyleManager::IsVehicleStyle() const
{
  return m_currentStyle.load()->name == kVehicleStyle;
}

ReaderPtr<Reader> MapStyleManager::GetDefaultResourceReader(std::string const & file)
{
  return GetPlatform().GetReader(base::JoinPath("styles", "common", file));
}

ReaderPtr<Reader> MapStyleManager::GetDrawingRulesReader() const
{
  MapStyleConfig const & config = *m_currentStyle.load();
  std::string const rulesFile = base::JoinPath(
      config.styleDir, m_currentTheme == MapStyleTheme::Light ? config.light.drulesPath : config.dark.drulesPath);
  return GetPlatform().GetReader(rulesFile);
}

ReaderPtr<Reader> MapStyleManager::GetSymbolsReader(std::string const & file, std::string_view density) const
{
  MapStyleConfig const & config = *m_currentStyle.load();
  std::string const resFile = base::JoinPath(
      config.styleDir, m_currentTheme == MapStyleTheme::Light ? config.light.symbolsPath : config.dark.symbolsPath,
      std::string{density}, file);

  return GetPlatform().GetReader(resFile);
}

ReaderPtr<Reader> MapStyleManager::GetTransitColorsReader() const
{
  MapStyleConfig const & config = *m_currentStyle.load();
  return GetPlatform().GetReader(base::JoinPath(config.styleDir, config.transitColorsPath));
}

std::string MapStyleManager::GetColorsPath() const
{
  MapStyleConfig const & config = *m_currentStyle.load();
  return base::JoinPath(config.styleDir, config.colorsPath);
}

std::string MapStyleManager::GetPatternsPath() const
{
  MapStyleConfig const & config = *m_currentStyle.load();
  return base::JoinPath(config.styleDir, config.patternsPath);
}

MapStyleManager::MapStyleManager()
{
  LoadStyles();
  // Defaults
  SetStyle(GetDefaultStyleName());
  SetTheme(MapStyleTheme::Light);
}

void MapStyleManager::LoadStyles()
{
  static std::string const stylesPath = base::JoinPath(GetPlatform().ResourcesDir(), kStylesDir);
  LOG(LWARNING, ("Loading map styles from", stylesPath));
  Platform::FilesList files;
  GetPlatform().GetFilesRecursivelyFromResources(kStylesDir, files);

  for (auto const & file : files)
    if (file.ends_with(kStyleFileName))
      LoadStyle(file);

  ASSERT(!m_styles.empty(), ());
  ASSERT(m_styles.contains("default"), ());
}

void MapStyleManager::LoadStyle(std::string const & path)
{
  LOG(LINFO, ("Loading style config from", path));
  std::string styleDir = base::GetDirectory(path);
  MapStyleConfig style;
  style.styleDir = styleDir;
  std::string buf;
  GetPlatform().GetReader(path)->ReadAsString(buf);
  if (auto const ec = glz::read_json(style, buf); ec.ec != glz::error_code::none)
  {
    LOG(LWARNING, ("Failed to parse style config from", path, "error:", ec.ec));
    return;
  }
  base::GetNameFromFullPath(styleDir);

  // This magic is needed to make unordered_map key's (string_view) data stored in unordered_map's value
  auto const [it, _] = m_styles.insert({{}, std::move(style)});
  auto nh = m_styles.extract(it);
  nh.key() = nh.mapped().name;
  m_styles.insert(std::move(nh));
}

MapStyleConfig const & MapStyleManager::GetStyle(MapStyleName const styleName) const
{
  if (m_styles.contains(styleName))
    return m_styles.at(styleName);

  LOG(LWARNING, ("Style", styleName, "not found. Returning default style"));
  return m_styles.at(GetDefaultStyleName());
}

std::string DebugPrint(MapStyleTheme const theme)
{
  switch (theme)
  {
  case MapStyleTheme::Count: LOG(LERROR, ("Wrong map theme param.")); [[fallthrough]];
  case MapStyleTheme::Dark: return "Dark";
  case MapStyleTheme::Light: return "Light";
  }
  UNREACHABLE();
}
