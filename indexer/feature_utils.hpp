#pragma once

#include <string>
#include <vector>

struct FeatureID;
class StringUtf8Multilang;

namespace feature
{
  // Address house numbers interpolation.
  enum class InterpolType { None, Odd, Even, Any };

  class TypesHolder;
  class RegionData;

  /// Get viewport scale to show given feature. Used in search.
  int GetFeatureViewportScale(TypesHolder const & types);

  // Returns following languages given |lang|:
  // - |lang|;
  // - languages that we know are similar to |lang|;
  std::vector<int8_t> GetSimilar(int8_t deviceLang);

  struct NameParamsIn
  {
    NameParamsIn(StringUtf8Multilang const & src_, RegionData const & regionData_,
                 int8_t deviceLang_, bool allowTranslit_)
      : src(src_), regionData(regionData_), deviceLang(deviceLang_), allowTranslit(allowTranslit_)
    {
    }
    NameParamsIn(StringUtf8Multilang const & src, RegionData const & regionData,
                 std::string_view deviceLang, bool allowTranslit);

    StringUtf8Multilang const & src;
    RegionData const & regionData;
    int8_t const deviceLang;
    bool allowTranslit;

    bool IsNativeOrSimilarLang() const;
  };

  struct NameParamsOut
  {
    /// In case when primary name is empty it will be propagated from secondary and secondary will be
    /// cleared. In case when primary name contains secondary name then secondary will be cleared.
    std::string_view primary, secondary;
    std::string transliterated;

    /// Call this fuction to get primary name when allowTranslit == true.
    std::string_view GetPrimary() const
    {
      return (!primary.empty() ? primary : std::string_view(transliterated));
    }

    void Clear()
    {
      primary = secondary = {};
      transliterated.clear();
    }
  };

  /// When the language of the device is equal to one of the languages of the MWM
  /// (or similar to device languages) only single name scheme is used. See GetReadableName method.
  /// Primary name using priority:
  /// - device language name;
  /// - languages that we know are similar to device language;
  /// - international name;
  /// - english name;
  /// - transliterated name (if allowed).
  /// Secondary name using priority:
  /// - default name;
  /// - international name;
  /// - country language name;
  /// - english name.
  void GetPreferredNames(NameParamsIn const & in, NameParamsOut & out);

  /// When MWM contains user's language (or similar to device languages if provided),
  /// the priority is the following:
  /// - device language name;
  /// - default name;
  /// - languages that we know are similar to device language;
  /// - international name;
  /// - english name;
  /// - transliterated name (if allowed);
  /// - country language name.
  /// When MWM does not contain user's language (or similar to device languages),
  /// the priority is the following:
  /// - device language name;
  /// - languages that we know are similar to device language;
  /// - international name;
  /// - english name;
  /// - transliterated name (if allowed);
  /// - default name;
  /// - country language name.
  void GetReadableName(NameParamsIn const & in, NameParamsOut & out);

  /// Returns language id as return result and name for search on booking in the @name parameter,
  ///  the priority is the following:
  /// - default name;
  /// - country language name;
  /// - english name.
  //int8_t GetNameForSearchOnBooking(RegionData const & regionData, StringUtf8Multilang const & src, std::string & name);

  /// Returns preferred name when only the device language is available.
  bool GetPreferredName(StringUtf8Multilang const & src, int8_t deviceLang, std::string_view & out);

  /// Returns priority list of language codes for feature description,
  /// the priority is the following:
  /// - device language code;
  /// - default language code if MWM contains user's language (or similar to device languages if provided);
  /// - languages that we know are similar to device language;
  /// - international language code;
  /// - english language code;
  std::vector<int8_t> GetDescriptionLangPriority(RegionData const & regionData, int8_t const deviceLang);

  // Returns vector of cuisines readable names from classificator.
  std::vector<std::string> GetCuisines(TypesHolder const & types);

  // Returns vector of cuisines names localized by platform.
  std::vector<std::string> GetLocalizedCuisines(TypesHolder const & types);

  // Returns vector of recycling types readable names from classificator.
  std::vector<std::string> GetRecyclingTypes(TypesHolder const & types);

  // Returns vector of recycling types localized by platform.
  std::vector<std::string> GetLocalizedRecyclingTypes(TypesHolder const & types);
}  // namespace feature
