#pragma once

#include "coding/reader.hpp"

#include "map_style.hpp"

#include <atomic>
#include <string>

class StyleReader
{
public:
  StyleReader();

  void SetCurrentStyle(MapStyle mapStyle);
  MapStyle GetCurrentStyle() const;
  bool IsCarNavigationStyle() const;

  ReaderPtr<Reader> GetDrawingRulesReader() const;

  ReaderPtr<Reader> GetResourceReader(std::string const & file, std::string_view density) const;
  ReaderPtr<Reader> GetDefaultResourceReader(std::string const & file) const;

private:
  std::atomic<MapStyle> m_mapStyle;
};

extern StyleReader & GetStyleReader();
