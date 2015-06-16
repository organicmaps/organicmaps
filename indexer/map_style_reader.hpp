#pragma once

#include "coding/reader.hpp"

#include "map_style.hpp"

class StyleReader
{
public:
  void SetCurrentStyle(MapStyle mapStyle);
  MapStyle GetCurrentStyle();

  ReaderPtr<Reader> GetDrawingRulesReader();

  ReaderPtr<Reader> GetResourceReader(string const & file, string const & density);
};

extern StyleReader & GetStyleReader();
