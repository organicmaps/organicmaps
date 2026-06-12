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

  // Reader for the bundled drawing-rules family file of mapStyle (drules_<family>.bin). Takes an
  // explicit style (not the current one) so a family can be loaded without flipping the global
  // current style, which background tile readers observe.
  ReaderPtr<Reader> GetDrawingRulesReader(MapStyle mapStyle) const;
  // Reads a WritableDir/styles/ override of that file into buffer if present (returns true).
  bool ReadDrawingRulesOverride(MapStyle mapStyle, std::string & buffer) const;
  // Index of mapStyle's variant inside its family file (light == 0, dark == 1).
  size_t GetDrawingRulesVariant(MapStyle mapStyle) const;

  ReaderPtr<Reader> GetResourceReader(std::string const & file, std::string_view density) const;
  ReaderPtr<Reader> GetDefaultResourceReader(std::string const & file) const;

private:
  std::atomic<MapStyle> m_mapStyle;
};

extern StyleReader & GetStyleReader();
