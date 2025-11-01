#pragma once

#include <atomic>

#include "styles/map_style_config.hpp"

#include "coding/reader.hpp"

#include <string_view>
#include <unordered_map>

using MapStyleName = std::string_view;

enum class MapStyleTheme : uint8_t
{
  Light = 0,
  Dark,

  Count,
};

class MapStyleManager
{
public:
  static MapStyleManager & Instance();

  void SetStyle(MapStyleName styleName);
  void SetTheme(MapStyleTheme theme);

  MapStyleName GetCurrentStyleName() const;
  std::vector<MapStyleName> GetAvailableStyles() const;

  MapStyleTheme GetCurrentTheme() const;

  bool IsValidStyle(MapStyleName styleName) const;
  bool IsVehicleStyle() const;

  static ReaderPtr<Reader> GetDefaultResourceReader(std::string const & file);

  ReaderPtr<Reader> GetDrawingRulesReader() const;
  ReaderPtr<Reader> GetSymbolsReader(std::string const & file, std::string_view density) const;
  ReaderPtr<Reader> GetTransitColorsReader() const;
  std::string GetColorsPath() const;
  std::string GetPatternsPath() const;

  static MapStyleName GetDefaultStyleName();
  static MapStyleName GetVehicleStyleName();
  static MapStyleName GetOutdoorsStyleName();
  static MapStyleName GetMergedStyleName();

private:
  MapStyleManager();

  void LoadStyles();
  void LoadStyle(std::string const & path);

  [[nodiscard]] MapStyleConfig const & GetStyle(MapStyleName styleName) const;

  std::unordered_map<MapStyleName, MapStyleConfig> m_styles;
  std::atomic<MapStyleConfig const *> m_currentStyle;
  std::atomic<MapStyleTheme> m_currentTheme;
};

std::string DebugPrint(MapStyleTheme theme);
