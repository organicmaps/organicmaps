#pragma once

#include "coding/reader.hpp"

#include "map_style.hpp"

class StyleReader
{
public:
  StyleReader();

  void SetCurrentStyle(MapStyle mapStyle);
  MapStyle GetCurrentStyle();

  ReaderPtr<Reader> GetDrawingRulesReader();

  ReaderPtr<Reader> GetResourceReader(string const & file, string const & density);

private:
  MapStyle m_mapStyle;
};

extern StyleReader & GetStyleReader();
