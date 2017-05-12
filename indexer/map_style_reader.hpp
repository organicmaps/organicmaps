#pragma once

#include "coding/reader.hpp"

#include "map_style.hpp"

#include <string>

class StyleReader
{
public:
  StyleReader();

  void SetCurrentStyle(MapStyle mapStyle);
  MapStyle GetCurrentStyle();

  ReaderPtr<Reader> GetDrawingRulesReader();

  ReaderPtr<Reader> GetResourceReader(std::string const & file, std::string const & density);
  ReaderPtr<Reader> GetDefaultResourceReader(std::string const & file);

private:
  MapStyle m_mapStyle;
};

extern StyleReader & GetStyleReader();
