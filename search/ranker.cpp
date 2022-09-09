#include "search/ranker.hpp"

#include "search/emitter.hpp"
#include "search/geometry_utils.hpp"
#include "search/highlighting.hpp"
#include "search/model.hpp"
#include "search/pre_ranking_info.hpp"
#include "search/ranking_utils.hpp"
#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/brands_holder.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <memory>
#include <optional>

namespace search
{
using namespace std;

namespace
{
template <typename Slice>
void UpdateNameScores(string_view name, uint8_t lang, Slice const & slice, NameScores & bestScores)
{
  if (lang == StringUtf8Multilang::kAltNameCode || lang == StringUtf8Multilang::kOldNameCode)
  {
    strings::Tokenize(name, ";", [&](string_view n)
    {
      bestScores.UpdateIfBetter(GetNameScores(n, lang, slice));
    });
  }
  else
  {
    bestScores.UpdateIfBetter(GetNameScores(name, lang, slice));
  }
}

template <typename Slice>
void UpdateNameScores(vector<strings::UniString> const & tokens, uint8_t lang, Slice const & slice,
                      NameScores & bestScores)
{
  bestScores.UpdateIfBetter(GetNameScores(tokens, lang, slice));
}

// This function supports only street names like "abcdstrasse"/"abcd strasse".
vector<vector<strings::UniString>> ModifyStrasse(vector<strings::UniString> const & streetTokens)
{
  vector<vector<strings::UniString>> result;
  auto static const kStrasse = strings::MakeUniString("strasse");
  auto static const kStr = strings::MakeUniString("str");
  auto const size = streetTokens.size();

  if (size == 0 || !strings::EndsWith(streetTokens.back(), kStrasse))
    return {};

  if (streetTokens.back() == kStrasse)
  {
    if (size == 1)
      return {};

    // "Abcd strasse" -> "abcdstrasse".
    result.emplace_back(streetTokens.begin(), streetTokens.end() - 1);
    result.back().back() += kStrasse;

    // "Abcd strasse" -> "abcdstr".
    result.emplace_back(streetTokens.begin(), streetTokens.end() - 1);
    result.back().back() += kStr;
    return result;
  }

  // "Abcdstrasse" -> "abcd strasse".
  auto const name =
      strings::UniString(streetTokens.back().begin(), streetTokens.back().end() - kStrasse.size());
  result.push_back(streetTokens);
  result.back().back() = name;
  result.back().push_back(kStrasse);

  // "Abcdstrasse" -> "abcdstr".
  result.push_back(streetTokens);
  result.back().back() = name + kStr;
  return result;
}

NameScores GetNameScores(FeatureType & ft, Geocoder::Params const & params,
                         TokenRange const & range, Model::Type type)
{
  NameScores bestScores;

  TokenSlice const slice(params, range);
  TokenSliceNoCategories const sliceNoCategories(params, range);

  for (auto const lang : params.GetLangs())
  {
    string_view const name = ft.GetName(lang);
    if (name.empty())
      continue;

    auto const updateScore = [&](string_view n)
    {
      vector<strings::UniString> t;
      PrepareStringForMatching(n, t);

      UpdateNameScores(t, lang, slice, bestScores);
      UpdateNameScores(t, lang, sliceNoCategories, bestScores);

      if (type == Model::TYPE_STREET)
      {
        auto const variants = ModifyStrasse(t);
        for (auto const & variant : variants)
        {
          UpdateNameScores(variant, lang, slice, bestScores);
          UpdateNameScores(variant, lang, sliceNoCategories, bestScores);
        }
      }
    };

    if (lang == StringUtf8Multilang::kAltNameCode || lang == StringUtf8Multilang::kOldNameCode)
    {
      strings::Tokenize(name, ";", [&updateScore](string_view n)
      {
        updateScore(n);
      });
    }
    else
    {
      updateScore(name);
    }
  }

  if (type == Model::TYPE_BUILDING)
    UpdateNameScores(ft.GetHouseNumber(), StringUtf8Multilang::kDefaultCode, sliceNoCategories,
                     bestScores);

  if (ftypes::IsAirportChecker::Instance()(ft))
  {
    auto const iata = ft.GetMetadata(feature::Metadata::FMD_AIRPORT_IATA);
    if (!iata.empty())
      UpdateNameScores(iata, StringUtf8Multilang::kDefaultCode, sliceNoCategories, bestScores);
  }

  auto const op = ft.GetMetadata(feature::Metadata::FMD_OPERATOR);
  if (!op.empty())
    UpdateNameScores(op, StringUtf8Multilang::kDefaultCode, sliceNoCategories, bestScores);

  auto const brand = ft.GetMetadata(feature::Metadata::FMD_BRAND);
  if (!brand.empty())
  {
    auto const & brands = indexer::GetDefaultBrands();
    /// @todo Avoid temporary string when unordered_map will allow search by string_view.
    brands.ForEachNameByKey(std::string(brand), [&](indexer::BrandsHolder::Brand::Name const & name)
    {
      UpdateNameScores(name.m_name, name.m_locale, sliceNoCategories, bestScores);
    });
  }

  if (type == Model::TYPE_STREET)
  {
    for (auto const & shield : feature::GetRoadShieldsNames(ft.GetRoadNumber()))
      UpdateNameScores(shield, StringUtf8Multilang::kDefaultCode, sliceNoCategories, bestScores);
  }

  return bestScores;
}

void MatchTokenRange(FeatureType & ft, Geocoder::Params const & params, TokenRange const & range,
                     Model::Type type, ErrorsMade & errorsMade, size_t & matchedLength,
                     bool & isAltOrOldName)
{
  auto const scores = GetNameScores(ft, params, range, type);
  errorsMade = scores.m_errorsMade;
  isAltOrOldName = scores.m_isAltOrOldName;
  matchedLength = scores.m_matchedLength;
  if (errorsMade.IsValid())
    return;
}

void RemoveDuplicatingLinear(vector<RankerResult> & results)
{
  double constexpr kDistSameStreetMeters = 5000.0;

  auto const lessCmp = [](RankerResult const & r1, RankerResult const & r2)
  {
    if (r1.GetGeomType() != r2.GetGeomType())
      return r1.GetGeomType() < r2.GetGeomType();

    auto const & ri1 = r1.GetRankingInfo();
    auto const & ri2 = r2.GetRankingInfo();

    if (ri1.m_type != ri2.m_type)
      return ri1.m_type < ri2.m_type;

    if (r1.GetName() != r2.GetName())
      return r1.GetName() < r2.GetName();

    if (ri1.m_type == Model::TYPE_STREET)
    {
      if (ri1.m_classifType.street != ri2.m_classifType.street)
        return ri1.m_classifType.street < ri2.m_classifType.street;
    }
    else
    {
      uint32_t const t1 = r1.GetBestType();
      uint32_t const t2 = r2.GetBestType();
      if (t1 != t2)
        return t1 < t2;
    }

    // After unique, the better feature should be kept.
    return r1.GetLinearModelRank() > r2.GetLinearModelRank();
  };

  auto const equalCmp = [](RankerResult const & r1, RankerResult const & r2)
  {
    if (r1.GetGeomType() != feature::GeomType::Line || !r1.IsEqualBasic(r2))
      return false;

    auto const & ri1 = r1.GetRankingInfo();
    auto const & ri2 = r2.GetRankingInfo();
    if (ri1.m_type == Model::TYPE_STREET)
    {
      if (ri1.m_classifType.street != ri2.m_classifType.street)
        return false;
    }
    else
    {
      if (r1.GetBestType() != r2.GetBestType())
        return false;
    }

    // Note! Do compare for distance when filtering linear objects.
    // Otherwise we will skip the results for different parts of the map.
    return PointDistance(r1.GetCenter(), r2.GetCenter()) < kDistSameStreetMeters;
  };

  base::SortUnique(results, lessCmp, equalCmp);
}

ftypes::LocalityType GetLocalityIndex(feature::TypesHolder const & types)
{
  using namespace ftypes;

  // Inner logic of SearchAddress expects COUNTRY, STATE and CITY only.
  LocalityType const type = IsLocalityChecker::Instance().GetType(types);
  switch (type)
  {
  case LocalityType::None:
  case LocalityType::Country:
  case LocalityType::State:
  case LocalityType::City: return type;
  case LocalityType::Town: return LocalityType::City;
  case LocalityType::Village: return LocalityType::None;
  case LocalityType::Count: return type;
  }
  UNREACHABLE();
}

// TODO: Format street and house number according to local country's rules.
string FormatStreetAndHouse(ReverseGeocoder::Address const & addr)
{
  return addr.GetStreetName() + ", " + addr.GetHouseNumber();
}

// TODO: Share common formatting code for search results and place page.
string FormatFullAddress(ReverseGeocoder::Address const & addr, string const & region)
{
  // TODO: Print "near" for not exact addresses.
  if (addr.GetDistance() != 0)
    return region;

  return FormatStreetAndHouse(addr) + (region.empty() ? "" : ", ") + region;
}

bool ResultExists(RankerResult const & p, vector<RankerResult> const & results,
                  double minDistanceOnMapBetweenResults)
{
  // Filter equal features in different mwms.
  auto equalCmp = [&p, &minDistanceOnMapBetweenResults](RankerResult const & r)
  {
    if (p.GetResultType() == r.GetResultType() &&
        p.GetResultType() == RankerResult::Type::Feature)
    {
      if (p.IsEqualCommon(r))
        return PointDistance(p.GetCenter(), r.GetCenter()) < minDistanceOnMapBetweenResults;
    }

    return false;
  };

  // Do not insert duplicating results.
  return find_if(results.begin(), results.end(), equalCmp) != results.cend();
}

class LazyAddressGetter
{
public:
  LazyAddressGetter(ReverseGeocoder const & reverseGeocoder, m2::PointD const & center)
    : m_reverseGeocoder(reverseGeocoder), m_center(center)
  {
  }

  ReverseGeocoder::Address const & GetNearbyAddress()
  {
    if (m_computedNearby)
      return m_address;
    m_reverseGeocoder.GetNearbyAddress(m_center, m_address);
    m_computedNearby = true;
    return m_address;
  }

  bool GetExactAddress(ReverseGeocoder::Address & address)
  {
    if (m_computedExact)
    {
      address = m_address;
      return true;
    }
    m_reverseGeocoder.GetNearbyAddress(m_center, 0.0, m_address);
    if (m_address.IsValid())
    {
      m_computedExact = true;
      m_computedNearby = true;
      address = m_address;
    }
    return m_computedExact;
  }

private:
  ReverseGeocoder const & m_reverseGeocoder;
  m2::PointD const m_center;
  ReverseGeocoder::Address m_address;
  bool m_computedExact = false;
  bool m_computedNearby = false;
};
}  // namespace

class RankerResultMaker
{
  ftypes::IsWayChecker const & m_wayChecker;
  ftypes::IsCapitalChecker const & m_capitalChecker;
  ftypes::IsCountryChecker const & m_countryChecker;

public:
  RankerResultMaker(Ranker & ranker, DataSource const & dataSource,
                    storage::CountryInfoGetter const & infoGetter,
                    ReverseGeocoder const & reverseGeocoder, Geocoder::Params const & params)
    : m_wayChecker(ftypes::IsWayChecker::Instance())
    , m_capitalChecker(ftypes::IsCapitalChecker::Instance())
    , m_countryChecker(ftypes::IsCountryChecker::Instance())
    , m_ranker(ranker)
    , m_dataSource(dataSource)
    , m_infoGetter(infoGetter)
    , m_reverseGeocoder(reverseGeocoder)
    , m_params(params)
  {
  }

  optional<RankerResult> operator()(PreRankerResult const & preResult)
  {
    m2::PointD center;
    string name;
    string country;

    auto ft = LoadFeature(preResult.GetId(), center, name, country);
    if (!ft)
      return {};

    RankerResult res(*ft, center, std::move(name), country);

    RankingInfo info;
    InitRankingInfo(*ft, center, preResult, info);

    if (info.m_type == Model::TYPE_STREET)
    {
      info.m_classifType.street = m_wayChecker.GetSearchRank(res.GetBestType());

      /// @see Arbat_Address test.
      // "2" is a NameScore::FULL_PREFIX for "2-й Обыденский переулок", which is *very* high,
      // and suppresses building's rank, matched by house number.
      if (info.m_nameScore > NameScore::SUBSTRING)
      {
        auto const & range = info.m_tokenRanges[info.m_type];
        if (range.Size() == 1 && m_params.IsNumberTokens(range))
          info.m_nameScore = NameScore::SUBSTRING;
      }
    }

    info.m_rank = NormalizeRank(info.m_rank, info.m_type, center, country,
                                m_capitalChecker(*ft), !info.m_allTokensUsed);

    if (preResult.GetInfo().m_isCommonMatchOnly)
    {
      // Count tokens in Feature's name.
      auto normalized = NormalizeAndSimplifyString(res.GetName());
      PreprocessBeforeTokenization(normalized);
      int count = 0;
      SplitUniString(normalized, [&count](strings::UniString const &)
      {
        ++count;
      }, Delimiters());

      // Factor is a number of the rest, not common matched tokens in Feature' name. Bigger is worse.
      info.m_commonTokensFactor = min(3, count - int(info.m_tokenRanges[info.m_type].Size()));
      ASSERT_GREATER_OR_EQUAL(info.m_commonTokensFactor, 0, ());
    }

    res.SetRankingInfo(info);
    if (m_params.m_useDebugInfo)
      res.m_dbgInfo = std::make_shared<RankingInfo>(std::move(info));

#ifdef SEARCH_USE_PROVENANCE
    res.m_provenance = preResult.GetProvenance();
#endif

    return res;
  }

private:
  bool IsSameLoader(FeatureID const & id) const
  {
    return (m_loader && m_loader->GetId() == id.m_mwmId);
  }

  unique_ptr<FeatureType> LoadFeature(FeatureID const & id)
  {
    if (!IsSameLoader(id))
      m_loader = make_unique<FeaturesLoaderGuard>(m_dataSource, id.m_mwmId);
    return LoadFeatureImpl(id, *m_loader);
  }

  static unique_ptr<FeatureType> LoadFeatureImpl(FeatureID const & id, FeaturesLoaderGuard & loader)
  {
    auto ft = loader.GetFeatureByIndex(id.m_index);
    if (ft)
      ft->SetID(id);
    return ft;
  }

  // For the best performance, incoming ids should be sorted by id.first (mwm file id).
  unique_ptr<FeatureType> LoadFeature(FeatureID const & id, m2::PointD & center, string & name,
                                      string & country)
  {
    auto ft = LoadFeature(id);
    if (!ft)
      return ft;

    // Country (region) name is a file name if feature isn't from World.mwm.
    ASSERT(m_loader && m_loader->GetId() == id.m_mwmId, ());
    if (m_loader->IsWorld())
      country.clear();
    else
      country = m_loader->GetCountryFileName();

    center = feature::GetCenter(*ft);
    m_ranker.GetBestMatchName(*ft, name);

    // Insert exact address (street and house number) instead of empty result name.
    if (name.empty())
    {
      ReverseGeocoder::Address addr;
      if (LazyAddressGetter(m_reverseGeocoder, center).GetExactAddress(addr))
      {
        unique_ptr<FeatureType> streetFeature;

        // We can't change m_loader here, because of the following RankerResult. So do this trick:
        if (IsSameLoader(addr.m_street.m_id))
        {
          streetFeature = LoadFeatureImpl(addr.m_street.m_id, *m_loader);
        }
        else
        {
          auto loader = make_unique<FeaturesLoaderGuard>(m_dataSource, addr.m_street.m_id.m_mwmId);
          streetFeature = LoadFeatureImpl(addr.m_street.m_id, *loader);
        }

        if (streetFeature)
        {
          string streetName;
          m_ranker.GetBestMatchName(*streetFeature, streetName);
          name = streetName + ", " + addr.GetHouseNumber();
        }
      }
    }

    return ft;
  }

  void InitRankingInfo(FeatureType & ft, m2::PointD const & center, PreRankerResult const & res, RankingInfo & info)
  {
    auto const & preInfo = res.GetInfo();
    auto const & pivot = m_ranker.m_params.m_pivot;

    feature::TypesHolder featureTypes(ft);

    info.m_distanceToPivot = mercator::DistanceOnEarth(center, pivot);
    info.m_rank = preInfo.m_rank;
    info.m_popularity = preInfo.m_popularity;
    info.m_type = preInfo.m_type;
    if (Model::IsPoi(info.m_type))
      info.m_classifType.poi = GetPoiType(featureTypes);
    info.m_allTokensUsed = preInfo.m_allTokensUsed;
    info.m_numTokens = m_params.GetNumTokens();
    info.m_exactMatch = preInfo.m_exactMatch;
    info.m_categorialRequest = m_params.IsCategorialRequest();
    info.m_tokenRanges = preInfo.m_tokenRanges;

    size_t totalLength = 0;
    for (size_t i = 0; i < m_params.GetNumTokens(); ++i)
      totalLength += m_params.GetToken(i).GetOriginal().size();
    // Avoid division by zero.
    if (totalLength == 0)
      totalLength = 1;

    if (m_params.IsCategorialRequest())
    {
      // We do not compare result name and request for categorial requests but we prefer named features.
      info.m_hasName = ft.HasName();
      if (!info.m_hasName)
      {
        info.m_hasName = ft.HasMetadata(feature::Metadata::FMD_OPERATOR) ||
                         ft.HasMetadata(feature::Metadata::FMD_BRAND);
      }
    }
    else
    {
      auto const scores = GetNameScores(ft, m_params, preInfo.InnermostTokenRange(), info.m_type);

      auto nameScore = scores.m_nameScore;
      auto errorsMade = scores.m_errorsMade;
      bool isAltOrOldName = scores.m_isAltOrOldName;
      auto matchedLength = scores.m_matchedLength;

      if (info.m_type != Model::TYPE_STREET &&
          preInfo.m_geoParts.m_street != IntersectionResult::kInvalidId)
      {
        auto const & mwmId = ft.GetID().m_mwmId;
        auto street = LoadFeature(FeatureID(mwmId, preInfo.m_geoParts.m_street));
        if (street)
        {
          auto const type = Model::TYPE_STREET;
          auto const & range = preInfo.m_tokenRanges[type];
          auto const streetScores = GetNameScores(*street, m_params, range, type);

          nameScore = min(nameScore, streetScores.m_nameScore);
          errorsMade += streetScores.m_errorsMade;
          if (streetScores.m_isAltOrOldName)
            isAltOrOldName = true;
          matchedLength += streetScores.m_matchedLength;
        }
      }

      if (info.m_type != Model::TYPE_SUBURB &&
          preInfo.m_geoParts.m_suburb != IntersectionResult::kInvalidId)
      {
        auto const & mwmId = ft.GetID().m_mwmId;
        auto suburb = LoadFeature(FeatureID(mwmId, preInfo.m_geoParts.m_suburb));
        if (suburb)
        {
          auto const type = Model::TYPE_SUBURB;
          auto const & range = preInfo.m_tokenRanges[type];
          ErrorsMade suburbErrors;
          size_t suburbMatchedLength = 0;
          bool suburbNameIsAltNameOrOldName = false;
          MatchTokenRange(*suburb, m_params, range, type, suburbErrors, suburbMatchedLength,
                          suburbNameIsAltNameOrOldName);
          errorsMade += suburbErrors;
          matchedLength += suburbMatchedLength;
          if (suburbNameIsAltNameOrOldName)
            isAltOrOldName = true;
        }
      }

      if (!Model::IsLocalityType(info.m_type) && preInfo.m_cityId.IsValid())
      {
        auto city = LoadFeature(preInfo.m_cityId);
        if (city)
        {
          auto const type = Model::TYPE_CITY;
          auto const & range = preInfo.m_tokenRanges[type];
          ErrorsMade cityErrors;
          size_t cityMatchedLength = 0;
          bool cityNameIsAltNameOrOldName = false;
          MatchTokenRange(*city, m_params, range, type, cityErrors, cityMatchedLength,
                          cityNameIsAltNameOrOldName);
          errorsMade += cityErrors;
          matchedLength += cityMatchedLength;
          if (cityNameIsAltNameOrOldName)
            isAltOrOldName = true;
        }
      }

      info.m_nameScore = nameScore;
      info.m_errorsMade = errorsMade;
      info.m_isAltOrOldName = isAltOrOldName;
      info.m_matchedFraction = matchedLength / static_cast<float>(totalLength);

      info.m_exactCountryOrCapital = info.m_errorsMade == ErrorsMade(0) && info.m_allTokensUsed &&
                                     info.m_nameScore == NameScore::FULL_MATCH &&
          // Upgrade _any_ capital rank, not only _true_ capital (=2).
          // For example, search Barcelona from Istanbul or vice-versa.
                                     (m_countryChecker(featureTypes) || m_capitalChecker(featureTypes));
    }

    CategoriesInfo const categoriesInfo(featureTypes,
                                        TokenSlice(m_params, preInfo.InnermostTokenRange()),
                                        m_ranker.m_params.m_categoryLocales, m_ranker.m_categories);

    info.m_pureCats = categoriesInfo.IsPureCategories();
    if (info.m_pureCats)
    {
      // Compare with previous values, in case if was assigned by street or locality.

      info.m_nameScore = NameScore::SUBSTRING;
      if (m_params.GetNumTokens() == preInfo.InnermostTokenRange().Size())
        info.m_nameScore = NameScore::FULL_PREFIX;

      info.m_matchedFraction = std::max(info.m_matchedFraction,
                                        categoriesInfo.GetMatchedLength() / static_cast<float>(totalLength));
      if (!info.m_errorsMade.IsValid())
        info.m_errorsMade = ErrorsMade(0);
    }
    info.m_falseCats = categoriesInfo.IsFalseCategories();
  }

  uint8_t NormalizeRank(uint8_t rank, Model::Type type, m2::PointD const & center,
                        string const & country, bool isCapital, bool isRelaxed)
  {
    if (isRelaxed)
      rank /= 5.0;

    switch (type)
    {
    case Model::TYPE_VILLAGE: return rank / 2.5;
    case Model::TYPE_CITY:
    {
      if (isCapital || m_ranker.m_params.m_viewport.IsPointInside(center))
        return base::Clamp(static_cast<int>(rank) * 2, 0, 0xFF);

      storage::CountryInfo info;
      if (country.empty())
        m_infoGetter.GetRegionInfo(center, info);
      else
        m_infoGetter.GetRegionInfo(country, info);
      if (info.IsNotEmpty() && info.m_name == m_ranker.m_params.m_pivotRegion)
        return base::Clamp(static_cast<int>(rank * 1.7), 0, 0xFF);

      // Fallthrough like "STATE" for cities without info.
    } [[fallthrough]];
    case Model::TYPE_STATE: return rank / 1.5;
    case Model::TYPE_COUNTRY: return rank;

    default: return rank;
    }
  }

  Ranker & m_ranker;
  DataSource const & m_dataSource;
  storage::CountryInfoGetter const & m_infoGetter;
  ReverseGeocoder const & m_reverseGeocoder;
  Geocoder::Params const & m_params;

  unique_ptr<FeaturesLoaderGuard> m_loader;
};

Ranker::Ranker(DataSource const & dataSource, CitiesBoundariesTable const & boundariesTable,
               storage::CountryInfoGetter const & infoGetter, KeywordLangMatcher & keywordsScorer,
               Emitter & emitter, CategoriesHolder const & categories,
               vector<Suggest> const & suggests, VillagesCache & villagesCache,
               base::Cancellable const & cancellable)
  : m_reverseGeocoder(dataSource)
  , m_cancellable(cancellable)
  , m_keywordsScorer(keywordsScorer)
  , m_localities(dataSource, boundariesTable, villagesCache)
  , m_dataSource(dataSource)
  , m_infoGetter(infoGetter)
  , m_emitter(emitter)
  , m_categories(categories)
  , m_suggests(suggests)
{
}

void Ranker::Init(Params const & params, Geocoder::Params const & geocoderParams)
{
  m_params = params;
  m_geocoderParams = geocoderParams;
  m_preRankerResults.clear();
  m_tentativeResults.clear();
}

void Ranker::Finish(bool cancelled)
{
  // The results should be updated by Geocoder before Finish call.
  m_emitter.Finish(cancelled);
}

Result Ranker::MakeResult(RankerResult rankerResult, bool needAddress, bool needHighlighting) const
{
  // todo(@m) Used because Result does not have a default constructor. Factor out?
  auto mk = [&](RankerResult & r) -> Result
  {
    string address;
    if (needAddress)
    {
      address = GetLocalizedRegionInfoForResult(rankerResult);

      // Format full address only for suitable results.
      if (ftypes::IsAddressObjectChecker::Instance()(rankerResult.GetTypes()))
      {
        address = FormatFullAddress(
              LazyAddressGetter(m_reverseGeocoder, rankerResult.GetCenter()).GetNearbyAddress(), address);
      }
    }

    string & name = rankerResult.m_str;

    switch (r.GetResultType())
    {
    case RankerResult::Type::Feature:
    case RankerResult::Type::Building:
    {
      auto const type = rankerResult.GetBestType(&m_params.m_preferredTypes);
      return Result(r.GetID(), r.GetCenter(), move(name), move(address), type, move(r.m_details));
    }
    case RankerResult::Type::LatLon: return Result(r.GetCenter(), move(name), move(address));
    case RankerResult::Type::Postcode: return Result(r.GetCenter(), move(name));
    }
    ASSERT(false, ("Bad RankerResult type:", static_cast<size_t>(r.GetResultType())));
    UNREACHABLE();
  };

  auto res = mk(rankerResult);

  if (needAddress &&
      ftypes::IsLocalityChecker::Instance().GetType(rankerResult.GetTypes()) == ftypes::LocalityType::None)
  {
    m_localities.GetLocality(res.GetFeatureCenter(), [&](LocalityItem const & item)
    {
      string_view city;
      if (item.GetReadableName(city))
        res.PrependCity(city);
    });
  }

  if (needHighlighting)
    HighlightResult(m_params.m_tokens, m_params.m_prefix, res);

  res.SetRankingInfo(rankerResult.m_dbgInfo);

#ifdef SEARCH_USE_PROVENANCE
  res.SetProvenance(move(rankerResult.m_provenance));
#endif

  return res;
}

void Ranker::SuggestStrings()
{
  // Prefix is only empty when tokens exceeds the max allowed. No point in giving suggestions then.
  if (m_params.m_prefix.empty() || !m_params.m_suggestsEnabled)
    return;

  string prologue = DropLastToken(m_params.m_query);

  for (auto const locale : m_params.m_categoryLocales)
    MatchForSuggestions(m_params.m_prefix, locale, prologue);
}

void Ranker::UpdateResults(bool lastUpdate)
{
  if (!lastUpdate)
    BailIfCancelled();

  MakeRankerResults();
  RemoveDuplicatingLinear(m_tentativeResults);
  if (m_tentativeResults.empty())
    return;

  if (m_params.m_viewportSearch)
  {
    sort(m_tentativeResults.begin(), m_tentativeResults.end(),
         base::LessBy(&RankerResult::GetDistanceToPivot));
  }
  else
  {
    /// @note Here is _reverse_ order sorting, because bigger is better.
    sort(m_tentativeResults.rbegin(), m_tentativeResults.rend(),
         base::LessBy(&RankerResult::GetLinearModelRank));

    ProcessSuggestions(m_tentativeResults);
  }

  // Emit feature results.
  size_t count = m_emitter.GetResults().GetCount();
  size_t i = 0;
  for (; i < m_tentativeResults.size(); ++i)
  {
    if (!lastUpdate && count >= m_params.m_batchSize && !m_params.m_viewportSearch &&
        !m_params.m_categorialRequest)
    {
      break;
    }

    if (!lastUpdate)
    {
      BailIfCancelled();

      // For categorial requests, it is usual that several batches arrive
      // in the first call to UpdateResults(). Emit them as soon as they are available
      // to improve responsiveness.
      if (count != 0 && count % m_params.m_batchSize == 0)
        m_emitter.Emit();
    }

    if (count >= m_params.m_limit)
      break;

    auto & rankerResult = m_tentativeResults[i];
    if (!m_params.m_viewportSearch)
      LOG(LDEBUG, (rankerResult));

    Result result = MakeResult(move(rankerResult), m_params.m_needAddress, m_params.m_needHighlighting);

    if (m_params.m_viewportSearch)
    {
      m_emitter.AddResultNoChecks(move(result));
      ++count;
    }
    else
    {
      if (m_emitter.AddResult(move(result)))
        ++count;
    }
  }

  /// @todo Use deque for m_tentativeResults?
  m_tentativeResults.erase(m_tentativeResults.begin(), m_tentativeResults.begin() + i);

  // The last update must be handled by a call to Finish().
  if (!lastUpdate)
  {
    BailIfCancelled();
    m_emitter.Emit();
  }
}

void Ranker::ClearCaches() { m_localities.ClearCache(); }

void Ranker::SetLocale(string const & locale)
{
  m_regionInfoGetter.SetLocale(locale);
}

void Ranker::LoadCountriesTree() { m_regionInfoGetter.LoadCountriesTree(); }

void Ranker::MakeRankerResults()
{
  RankerResultMaker maker(*this, m_dataSource, m_infoGetter, m_reverseGeocoder, m_geocoderParams);
  for (auto const & r : m_preRankerResults)
  {
    auto p = maker(r);
    if (!p)
      continue;

    if (m_geocoderParams.m_mode == Mode::Viewport &&
        !m_geocoderParams.m_pivot.IsPointInside(p->GetCenter()))
    {
      continue;
    }

    /// @todo Do not filter "equal" results by distance in Mode::Viewport mode.
    /// Strange when (for example bus stops) not all results are highlighted.
    /// @todo Is it ok to make duplication check for O(N) here?
    /// Especially when we make RemoveDuplicatingLinear later.
    if (!ResultExists(*p, m_tentativeResults, m_params.m_minDistanceBetweenResultsM))
      m_tentativeResults.push_back(move(*p));
  };

  m_preRankerResults.clear();
}

void Ranker::GetBestMatchName(FeatureType & f, string & name) const
{
  int8_t bestLang = StringUtf8Multilang::kUnsupportedLanguageCode;
  KeywordLangMatcher::Score bestScore;
  auto updateScore = [&](int8_t lang, string_view s, bool force)
  {
    // Ignore name for categorial requests.
    auto const score = m_keywordsScorer.CalcScore(lang, m_params.m_categorialRequest ? "" : s);
    if (force ? bestScore <= score : bestScore < score)
    {
      bestScore = score;
      name = s;
      bestLang = lang;
    }
  };

  auto bestNameFinder = [&](int8_t lang, string_view s)
  {
    if (lang == StringUtf8Multilang::kAltNameCode || lang == StringUtf8Multilang::kOldNameCode)
    {
      strings::Tokenize(s, ";", [lang, &updateScore](std::string_view n)
      {
        updateScore(lang, n, true /* force */);
      });
    }
    else
    {
      updateScore(lang, s, true /* force */);
    }

    // Default name should be written in the regional language.
    if (lang == StringUtf8Multilang::kDefaultCode)
    {
      auto const mwmInfo = f.GetID().m_mwmId.GetInfo();
      vector<int8_t> mwmLangCodes;
      mwmInfo->GetRegionData().GetLanguages(mwmLangCodes);
      for (auto const l : mwmLangCodes)
        updateScore(l, s, false /* force */);
    }
  };
  UNUSED_VALUE(f.ForEachName(bestNameFinder));

  if (bestLang == StringUtf8Multilang::kAltNameCode ||
      bestLang == StringUtf8Multilang::kOldNameCode)
  {
    string_view const readableName = f.GetReadableName();
    // Do nothing if alt/old name is the only name we have.
    if (readableName != name && !readableName.empty())
      name = std::string(readableName) + " (" + name + ")";
  }
}

void Ranker::MatchForSuggestions(strings::UniString const & token, int8_t locale,
                                 string const & prologue)
{
  for (auto const & suggest : m_suggests)
  {
    strings::UniString const & s = suggest.m_name;
    if (suggest.m_prefixLength <= token.size()
        && token != s                  // do not push suggestion if it already equals to token
        && suggest.m_locale == locale  // push suggestions only for needed language
        && strings::StartsWith(s, token))
    {
      string const utf8Str = strings::ToUtf8(s);
      Result r(utf8Str, prologue + utf8Str + " ");
      HighlightResult(m_params.m_tokens, m_params.m_prefix, r);
      m_emitter.AddResult(move(r));
    }
  }
}

void Ranker::ProcessSuggestions(vector<RankerResult> const & vec) const
{
  if (m_params.m_prefix.empty() || !m_params.m_suggestsEnabled)
    return;

  size_t added = 0;
  for (RankerResult const & r : vec)
  {
    if (added >= kMaxNumSuggests)
      break;

    ftypes::LocalityType const type = GetLocalityIndex(r.GetTypes());
    if (type == ftypes::LocalityType::Country || type == ftypes::LocalityType::City || r.IsStreet())
    {
      string suggestion = GetSuggestion(r, m_params.m_query, m_params.m_tokens, m_params.m_prefix);
      if (!suggestion.empty())
      {
        // todo(@m) RankingInfo is lost here. Should it be?
        if (m_emitter.AddResult(Result(MakeResult(r, false /* needAddress */, true /* needHighlighting */),
                                       move(suggestion))))
        {
          ++added;
        }
      }
    }
  }
}

string Ranker::GetLocalizedRegionInfoForResult(RankerResult const & result) const
{
  auto const type = result.GetBestType(&m_params.m_preferredTypes);

  storage::CountryId id;
  if (!result.GetCountryId(m_infoGetter, type, id))
    return {};

  return m_regionInfoGetter.GetLocalizedFullName(id);
}
}  // namespace search
