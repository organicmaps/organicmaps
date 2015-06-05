#pragma once

#include "coding/reader.hpp"

#include "graphics/defines.hpp"

#include "map_style.hpp"

class StyleReader
{
public:
  void SetCurrentStyle(MapStyle mapStyle);
  MapStyle GetCurrentStyle();

  ReaderPtr<Reader> GetDrawingRulesReader();

  ReaderPtr<Reader> GetResourceReader(string const & file, graphics::EDensity density);
};

extern StyleReader & GetStyleReader();
