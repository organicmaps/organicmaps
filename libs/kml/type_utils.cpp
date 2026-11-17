#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_utils.hpp"

#include "platform/localization.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <limits>

namespace kml
{
bool IsEqual(m2::PointD const & lhs, m2::PointD const & rhs)
{
  return lhs.EqualDxDy(rhs, kMwmPointAccuracy);
}

bool IsEqual(geometry::PointWithAltitude const & lhs, geometry::PointWithAltitude const & rhs)
{
  return AlmostEqualAbs(lhs, rhs, kMwmPointAccuracy);
}

std::string GetPreferredBookmarkStr(LocalizableString const & name, std::string const & languageNorm)
{
  if (name.size() == 1)
    return name.begin()->second;

  // "default" requests a device-independent value for serialization and other stable output.
  // Resolve it directly to preserve an int_name verbatim; feature-name rendering deliberately
  // shortens comma-separated int_name values and therefore has different semantics.
  if (languageNorm == StringUtf8Multilang::GetLangByCode(kDefaultLangCode))
    return std::string{GetStringForExport(name)};

  /// @todo Complicated logic here when transforming LocalizableString -> StringUtf8Multilang to call GetPreferredName.
  StringUtf8Multilang nameMultilang;
  for (auto const & pair : name)
    nameMultilang.AddString(pair.first, pair.second);

  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languageNorm);

  std::string_view preferredName;
  if (feature::GetPreferredName(nameMultilang, deviceLang, preferredName))
    return std::string(preferredName);

  // A category or track has no feature type to fall back to, so keep every non-empty localized
  // string visible even when none of the preferred languages is present.
  return std::string{GetStringForExport(name)};
}

std::string GetPreferredBookmarkStr(LocalizableString const & name, feature::RegionData const & regionData,
                                    std::string const & languageNorm)
{
  if (name.size() == 1)
    return name.begin()->second;

  /// @todo Complicated logic here when transforming LocalizableString -> StringUtf8Multilang to call GetPreferredName.
  StringUtf8Multilang nameMultilang;
  for (auto const & pair : name)
    nameMultilang.AddString(pair.first, pair.second);

  feature::NameParamsOut out;
  feature::GetReadableName({nameMultilang, regionData, languageNorm, false /* allowTranslit */}, out);
  if (!out.primary.empty())
    return std::string(out.primary);

  // Keep the value visible when neither the device nor region languages are present.
  return std::string{GetStringForExport(name)};
}

std::string GetLocalizedFeatureType(std::vector<uint32_t> const & types)
{
  if (types.empty())
    return {};

  return platform::GetLocalizedTypeName(classif().GetReadableObjectName(types.front()));
}

std::string GetPreferredBookmarkName(BookmarkData const & bmData, std::string_view languageOrig)
{
  auto const languageNorm = languages::Normalize(languageOrig);
  std::string name = GetPreferredBookmarkStr(bmData.m_customName, languageNorm);
  if (name.empty())
    name = GetPreferredBookmarkStr(bmData.m_name, languageNorm);
  if (name.empty())
    name = GetLocalizedFeatureType(bmData.m_featureTypes);
  return name;
}

std::string_view GetStringForExport(LocalizableString const & lstr)
{
  // Rank of a language in the export priority list, the lowest one wins.
  auto const rank = [](int8_t lang)
  {
    // alt_name/old_name are OSM pseudo-names, not translations: an old name is worse than any
    // real one, so push both behind every language instead of letting their codes (53/55) win
    // over kk/mr/et/ku/mn/mk/lv/hi (56..63).
    int constexpr kPseudoNamePenalty = StringUtf8Multilang::kMaxSupportedLanguages;
    switch (lang)
    {
    case kDefaultLangCode: return 0;
    case StringUtf8Multilang::kInternationalCode: return 1;
    case StringUtf8Multilang::kEnglishCode: return 2;
    // Any other language is a deterministic last resort, ordered by its stable code.
    default: return 3 + lang + (StringUtf8Multilang::IsAltOrOldName(lang) ? kPseudoNamePenalty : 0);
    }
  };

  std::string_view best;
  int bestRank = std::numeric_limits<int>::max();
  for (auto const & [lang, value] : lstr)
    if (!value.empty())
      if (auto const r = rank(lang); r < bestRank)
        bestRank = r, best = value;

  return best;
}

}  // namespace kml
