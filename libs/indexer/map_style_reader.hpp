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

  // Designer mode (desktop app --designer, generator_tool --designer): style files rebuilt by the
  // Designer are read from the writable dir without the styles/ override, drawable scale ranges are
  // widened (see scales_patch.hpp) and the classificator is reloaded on every style refresh. Set
  // once at startup, before any style file is read.
  void SetDesignerMode(bool designerMode) { m_designerMode = designerMode; }
  bool IsDesignerMode() const { return m_designerMode; }

  // Reader for the bundled drawing-rules family file of mapStyle (drules_<family>.bin). Takes an
  // explicit style (not the current one) so a family can be loaded without flipping the global
  // current style, which background tile readers observe.
  ReaderPtr<Reader> GetDrawingRulesReader(MapStyle mapStyle) const;
  // Reads a WritableDir/styles/ override of that file into buffer if present (returns true). Always
  // returns false in Designer mode, which rebuilds the bundled file in the writable dir directly and
  // must not be shadowed by a stale override.
  bool ReadDrawingRulesOverride(MapStyle mapStyle, std::string & buffer) const;
  // Index of mapStyle's variant inside its family file (light == 0, dark == 1).
  size_t GetDrawingRulesVariant(MapStyle mapStyle) const;

  ReaderPtr<Reader> GetResourceReader(std::string const & file, std::string_view density) const;
  ReaderPtr<Reader> GetDefaultResourceReader(std::string const & file) const;

private:
  std::atomic<MapStyle> m_mapStyle;
  bool m_designerMode = false;
};

extern StyleReader & GetStyleReader();
