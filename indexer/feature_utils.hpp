#pragma once

#include <string>
#include <vector>

#include "indexer/ftraits.hpp"
#include "indexer/yes_no_unknown.hpp"

struct FeatureID;
class StringUtf8Multilang;

namespace feature
{
static constexpr uint8_t kMaxStarsCount = 7;
static constexpr std::string_view kFieldsSeparator = " • ";
static constexpr std::string_view kToiletsSymbol = "🚻";
static constexpr std::string_view kAtmSymbol = "💳";
static constexpr std::string_view kWheelchairSymbol = "♿️";
static constexpr std::string_view kWifiSymbol = "🛜";

/// OSM internet_access tag values.
enum class Internet
{
  Unknown,   //!< Internet state is unknown (default).
  Wlan,      //!< Wireless Internet access is present.
  Terminal,  //!< A computer with internet service.
  Wired,     //!< Wired Internet access is present.
  Yes,       //!< Unspecified Internet access is available.
  No         //!< There is definitely no any Internet access.
};
std::string DebugPrint(Internet internet);
/// @param[in]  inet  Should be lowercase like in DebugPrint.
Internet InternetFromString(std::string_view inet);

YesNoUnknown YesNoUnknownFromString(std::string_view str);

// Address house numbers interpolation.
enum class InterpolType : uint8_t
{
  None,
  Odd,
  Even,
  Any
};

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
  NameParamsIn(StringUtf8Multilang const & src_, RegionData const & regionData_, int8_t deviceLang_,
               bool allowTranslit_)
    : src(src_)
    , regionData(regionData_)
    , deviceLang(deviceLang_)
    , allowTranslit(allowTranslit_)
  {}
  NameParamsIn(StringUtf8Multilang const & src, RegionData const & regionData, std::string_view deviceLang,
               bool allowTranslit);

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
  std::string_view GetPrimary() const { return (!primary.empty() ? primary : std::string_view(transliterated)); }

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
// int8_t GetNameForSearchOnBooking(RegionData const & regionData, StringUtf8Multilang const & src, std::string & name);

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

// Returns fee type localized by platform.
std::string GetLocalizedFeeType(TypesHolder const & types);

// Returns readable wheelchair type.
std::string GetReadableWheelchairType(TypesHolder const & types);

/// @returns wheelchair availability.
std::optional<ftraits::WheelchairAvailability> GetWheelchairType(TypesHolder const & types);

/// Returns true if feature has ATM type.
bool HasAtm(TypesHolder const & types);

/// Returns true if feature has Toilets type.
bool HasToilets(TypesHolder const & types);

/// @returns formatted drinking water type.
std::string FormatDrinkingWater(TypesHolder const & types);

/// @returns starsCount of ★ symbol.
std::string FormatStars(uint8_t starsCount);

/// @returns formatted elevation with ▲ symbol and units.
std::string FormatElevation(std::string_view elevation);

}  // namespace feature
