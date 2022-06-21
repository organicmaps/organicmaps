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

  ReaderPtr<Reader> GetDrawingRulesReader();

  ReaderPtr<Reader> GetResourceReader(std::string const & file, std::string const & density) const;
  ReaderPtr<Reader> GetDefaultResourceReader(std::string const & file) const;

  // If enabled allows to load custom style files
  // from the "styles/" subdir of the writable dir.
  inline void SetStylesOverride(bool enabled)
  {
    m_isStylesOverrideEnabled = enabled;
  }

  inline bool IsStylesOverrideEnabled() const
  {
    return m_isStylesOverrideEnabled;
  }

  // If enabled allows rendering of features at up to 3 lower zoom levels
  // by reading extra scale/visibility indices and using a geometry fallback.
  inline void SetVisibilityOverride(bool enabled)
  {
    m_isVisibilityOverrideEnabled = enabled;
  }

  inline bool IsVisibilityOverrideEnabled() const
  {
    return m_isVisibilityOverrideEnabled;
  }

private:
  bool m_isStylesOverrideEnabled = true;
#ifdef BUILD_DESIGNER
  bool m_isVisibilityOverrideEnabled = true;
#else
  bool m_isVisibilityOverrideEnabled = false;
#endif

  std::atomic<MapStyle> m_mapStyle;
};

extern StyleReader & GetStyleReader();
