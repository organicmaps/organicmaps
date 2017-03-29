#pragma once

#include "geometry/rect2d.hpp"

#include "base/base.hpp"

struct FeatureID;
class StringUtf8Multilang;

namespace feature
{
  class TypesHolder;
  class RegionData;

  /// Get viewport scale to show given feature. Used in search.
  int GetFeatureViewportScale(TypesHolder const & types);

  /// Primary name using priority:
  /// - device language name;
  /// - international name;
  /// - english name;
  /// - transliterated name (if allowed).
  /// Secondary name using priority:
  /// - default name;
  /// - international name;
  /// - country language name;
  /// - english name.
  /// In case when primary name is empty it will be propagated from secondary and secondary will be
  /// cleared. In case when primary name contains secondary name then secondary will be cleared.
  void GetPreferredNames(RegionData const & regionData, StringUtf8Multilang const & src,
                         int8_t const deviceLang, bool allowTranslit, string & primary, string & secondary);

  /// When MWM contains user's language, the priority is the following:
  /// - device language name;
  /// - default name;
  /// - international name;
  /// - english name;
  /// - transliterated name (if allowed);
  /// - country language name.
  /// When MWM does not contain user's language, the priority is the following:
  /// - device language name;
  /// - international name;
  /// - english name;
  /// - transliterated name (if allowed);
  /// - default name;
  /// - country language name.
  void GetReadableName(RegionData const & regionData, StringUtf8Multilang const & src,
                       int8_t const deviceLang, bool allowTranslit, string & out);
}  // namespace feature
