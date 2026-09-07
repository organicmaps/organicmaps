#pragma once

#include <QtCore/QString>

#include <array>

namespace build_style
{
// DPI buckets of the symbols/<dpi>/<theme>/ atlases. Default symbol sizes can
// be overridden per bucket by a resolutions.txt file in the style directory.
struct SkinDpi
{
  char const * m_name;
  int m_size;
};

inline constexpr std::array<SkinDpi, 6> kSkinDpis = {
    {{"mdpi", 18}, {"hdpi", 27}, {"xhdpi", 36}, {"6plus", 43}, {"xxhdpi", 54}, {"xxxhdpi", 64}}};

// `theme` is "light" or "dark"; selects the symbols/<dpi>/<theme>/ output dir.
void BuildSkins(QString const & styleDir, QString const & outputDir, QString const & theme);
void ApplySkins(QString const & outputDir, QString const & theme);
}  // namespace build_style
