#pragma once

#include "indexer/map_style.hpp"

#include <QtCore/QString>

namespace build_style
{
// Identifies which MapStyle the Designer is editing.  Populated by
// TryParseStyleInfo() from the opened style.mapcss path.
struct StyleInfo
{
  MapStyle m_mapStyle = kDefaultMapStyle;
  QString m_styleType;     // "default" | "outdoors" | "vehicle"
  QString m_theme;         // "light" | "dark"
  QString m_drulesSuffix;  // e.g. "_default_light" — matches StyleReader's suffixes.
  QString m_includeDir;    // <stylesRoot>/<type>/include/, absolute, with trailing slash.
};

// Returns true and fills `out` if `mapcssFile` matches
// .../styles/<type>/<theme>/style.mapcss for one of the supported
// (type, theme) combinations.
bool TryParseStyleInfo(QString const & mapcssFile, StyleInfo & out);

void BuildAndApply(QString const & mapcssFile, StyleInfo const & info);
void BuildIfNecessaryAndApply(QString const & mapcssFile, StyleInfo const & info);
void RunRecalculationGeometryScript(QString const & mapcssFile);

extern bool NeedRecalculate;
}  // namespace build_style
