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
static constexpr std::string_view kWheelchairSymbol = "♿";
static constexpr std::string_view kWifiSymbol = "🛜";
static constexpr std::string_view kCarSymbol = "🚘";
static constexpr std::string_view kBicycleSymbol = "🚲";
static constexpr std::string_view kMotorcycleSymbol = "🏍";
static constexpr std::string_view kLevelSymbol = "🛗";

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
int GetFeatureViewportScale(FeatureID const & fid, TypesHolder const & types);

// Returns following languages given |lang|:
// - |lang|;
// - languages that we know are similar to |lang|;
LangsBufferT GetSimilar(int8_t deviceLang);

// Returns the first language declared for the MWM region, or kUnsupportedLanguageCode if the
// region declares none. Used as a HarfBuzz `locl` hint for text in the region's on-the-ground
// language: OSM-verbatim strings (addr:housenumber, road shield ref) and unqualified `name=`
// tags, whose selected code (kDefaultCode) carries no real BCP-47 language.
int8_t GetRegionLang(RegionData const & regionData);

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

  // Raw StringUtf8Multilang code actually selected for primary/secondary, used downstream to
  // drive HarfBuzz OpenType `locl` substitutions. kEnglishCode is reported for the transliterated
  // fallback (target script is Latin); kUnsupportedLanguageCode means no name was found at all.
  // An unqualified `name=` is reported verbatim as kDefaultCode, which carries no real BCP-47
  // language: resolve it to the region's language (see feature::GetRegionLang) at the usage place.
  int8_t primaryLang = StringUtf8Multilang::kUnsupportedLanguageCode;
  int8_t secondaryLang = StringUtf8Multilang::kUnsupportedLanguageCode;

  /// Call this fuction to get primary name when allowTranslit == true.
  std::string_view GetPrimary() const { return (!primary.empty() ? primary : std::string_view(transliterated)); }

  void Clear()
  {
    primary = secondary = {};
    transliterated.clear();
    primaryLang = secondaryLang = StringUtf8Multilang::kUnsupportedLanguageCode;
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
LangsBufferT GetDescriptionLangPriority(RegionData const & regionData, int8_t const deviceLang);

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

/// @returns formatted capacity with car/bicycle emoji.
std::string FormatCapacity(std::string_view capacity, TypesHolder const & types);

/// @returns formatted building level with level symbol.
std::string FormatLevel(std::string_view level);

}  // namespace feature
