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

using namespace std;

namespace search
{
namespace
{
template <typename Slice>
void UpdateNameScores(string const & name, uint8_t lang, Slice const & slice,
                      NameScores & bestScores)
{
  if (lang == StringUtf8Multilang::kAltNameCode || lang == StringUtf8Multilang::kOldNameCode)
  {
    auto const names = strings::Tokenize(name, ";");
    for (auto const & n : names)
      bestScores.UpdateIfBetter(GetNameScores(n, lang, slice));
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

pair<NameScores, size_t> GetNameScores(FeatureType & ft, Geocoder::Params const & params,
                                       TokenRange const & range, Model::Type type)
{
  NameScores bestScores;

  TokenSlice const slice(params, range);
  TokenSliceNoCategories const sliceNoCategories(params, range);

  size_t matchedLength = 0;
  if (type != Model::Type::TYPE_COUNT)
  {
    for (size_t i = 0; i < slice.Size(); ++i)
      matchedLength += slice.Get(i).GetOriginal().size();
  }

  for (auto const lang : params.GetLangs())
  {
    string name;
    if (!ft.GetName(lang, name))
      continue;
    vector<vector<strings::UniString>> tokens(1);
    if (lang == StringUtf8Multilang::kAltNameCode || lang == StringUtf8Multilang::kOldNameCode)
    {
      auto const names = strings::Tokenize(name, ";");
      tokens.resize(names.size());
      for (size_t i = 0; i < names.size(); ++i)
        PrepareStringForMatching(names[i], tokens[i]);
    }
    else
    {
      PrepareStringForMatching(name, tokens[0]);
    }

    for (auto const & t : tokens)
    {
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
    }
  }

  if (type == Model::TYPE_BUILDING)
    UpdateNameScores(ft.GetHouseNumber(), StringUtf8Multilang::kDefaultCode, sliceNoCategories,
                     bestScores);

  if (ftypes::IsAirportChecker::Instance()(ft))
  {
    string const iata = ft.GetMetadata(feature::Metadata::FMD_AIRPORT_IATA);
    if (!iata.empty())
      UpdateNameScores(iata, StringUtf8Multilang::kDefaultCode, sliceNoCategories, bestScores);
  }

  string const op = ft.GetMetadata(feature::Metadata::FMD_OPERATOR);
  if (!op.empty())
    UpdateNameScores(op, StringUtf8Multilang::kDefaultCode, sliceNoCategories, bestScores);

  string const brand = ft.GetMetadata(feature::Metadata::FMD_BRAND);
  if (!brand.empty())
  {
    auto const & brands = indexer::GetDefaultBrands();
    brands.ForEachNameByKey(brand, [&](indexer::BrandsHolder::Brand::Name const & name) {
      UpdateNameScores(name.m_name, name.m_locale, sliceNoCategories, bestScores);
    });
  }

  if (type == Model::TYPE_STREET)
  {
    for (auto const & shield : feature::GetRoadShieldsNames(ft.GetRoadNumber()))
      UpdateNameScores(shield, StringUtf8Multilang::kDefaultCode, sliceNoCategories, bestScores);
  }

  return make_pair(bestScores, matchedLength);
}

void MatchTokenRange(FeatureType & ft, Geocoder::Params const & params, TokenRange const & range,
                     Model::Type type, ErrorsMade & errorsMade, size_t & matchedLength,
                     bool & isAltOrOldName)
{
  auto const scores = GetNameScores(ft, params, range, type);
  errorsMade = scores.first.m_errorsMade;
  isAltOrOldName = scores.first.m_isAltOrOldName;
  matchedLength = scores.second;
  if (errorsMade.IsValid())
    return;

  for (auto const token : range)
  {
    errorsMade += ErrorsMade{GetMaxErrorsForToken(params.GetToken(token).GetOriginal())};
    matchedLength += params.GetToken(token).GetOriginal().size();
  }
}

void RemoveDuplicatingLinear(vector<RankerResult> & results)
{
  double constexpr kDistSameStreetMeters = 5000.0;

  auto lessCmp = [](RankerResult const & r1, RankerResult const & r2) -> bool {
    if (r1.GetGeomType() != r2.GetGeomType())
      return r1.GetGeomType() < r2.GetGeomType();

    if (r1.GetName() != r2.GetName())
      return r1.GetName() < r2.GetName();

    uint32_t const t1 = r1.GetBestType();
    uint32_t const t2 = r2.GetBestType();
    if (t1 != t2)
      return t1 < t2;

    // After unique, the better feature should be kept.
    return r1.GetLinearModelRank() > r2.GetLinearModelRank();
  };

  auto equalCmp = [](RankerResult const & r1, RankerResult const & r2) -> bool {
    // Note! Do compare for distance when filtering linear objects.
    // Otherwise we will skip the results for different parts of the map.
    return r1.GetGeomType() == feature::GeomType::Line && r1.IsEqualCommon(r2) &&
           PointDistance(r1.GetCenter(), r2.GetCenter()) < kDistSameStreetMeters;
  };

  sort(results.begin(), results.end(), lessCmp);
  results.erase(unique(results.begin(), results.end(), equalCmp), results.end());
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
  auto equalCmp = [&p, &minDistanceOnMapBetweenResults](RankerResult const & r) -> bool {
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
public:
  RankerResultMaker(Ranker & ranker, DataSource const & dataSource,
                    storage::CountryInfoGetter const & infoGetter,
                    ReverseGeocoder const & reverseGeocoder, Geocoder::Params const & params)
    : m_ranker(ranker)
    , m_dataSource(dataSource)
    , m_infoGetter(infoGetter)
    , m_reverseGeocoder(reverseGeocoder)
    , m_params(params)
  {
  }

  optional<RankerResult> operator()(PreRankerResult const & preRankerResult)
  {
    m2::PointD center;
    string name;
    string country;

    auto ft = LoadFeature(preRankerResult.GetId(), center, name, country);
    if (!ft)
      return {};

    RankerResult r(*ft, center, m_ranker.m_params.m_pivot, name, country);

    search::RankingInfo info;
    InitRankingInfo(*ft, center, preRankerResult, info);
    info.m_rank = NormalizeRank(info.m_rank, info.m_type, center, country,
                                ftypes::IsCapitalChecker::Instance()(*ft), !info.m_allTokensUsed);
    r.SetRankingInfo(move(info));
    r.m_provenance = move(preRankerResult.GetProvenance());

    return r;
  }

private:
  unique_ptr<FeatureType> LoadFeature(FeatureID const & id)
  {
    if (!m_loader || m_loader->GetId() != id.m_mwmId)
      m_loader = make_unique<FeaturesLoaderGuard>(m_dataSource, id.m_mwmId);
    auto ft = m_loader->GetFeatureByIndex(id.m_index);
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

    center = feature::GetCenter(*ft);
    m_ranker.GetBestMatchName(*ft, name);

    // Insert exact address (street and house number) instead of empty result name.
    if (name.empty())
    {
      ReverseGeocoder::Address addr;
      LazyAddressGetter addressGetter(m_reverseGeocoder, center);
      if (addressGetter.GetExactAddress(addr))
      {
        if (auto streetFeature = LoadFeature(addr.m_street.m_id))
        {
          string streetName;
          m_ranker.GetBestMatchName(*streetFeature, streetName);
          name = streetName + ", " + addr.GetHouseNumber();
        }
      }
    }

    // Country (region) name is a file name if feature isn't from
    // World.mwm.
    ASSERT(m_loader && m_loader->GetId() == id.m_mwmId, ());
    if (m_loader->IsWorld())
      country.clear();
    else
      country = m_loader->GetCountryFileName();

    return ft;
  }

  void InitRankingInfo(FeatureType & ft, m2::PointD const & center, PreRankerResult const & res,
                       search::RankingInfo & info)
  {
    auto const & preInfo = res.GetInfo();

    auto const & pivot = m_ranker.m_params.m_accuratePivotCenter;

    info.m_distanceToPivot = mercator::DistanceOnEarth(center, pivot);
    info.m_rank = preInfo.m_rank;
    info.m_popularity = preInfo.m_popularity;
    info.m_rating = preInfo.m_rating;
    info.m_type = preInfo.m_type;
    if (Model::IsPoi(info.m_type))
      info.m_resultType = GetResultType(feature::TypesHolder(ft));
    info.m_allTokensUsed = preInfo.m_allTokensUsed;
    info.m_numTokens = m_params.GetNumTokens();
    info.m_exactMatch = preInfo.m_exactMatch;
    info.m_categorialRequest = m_params.IsCategorialRequest();
    info.m_tokenRanges = preInfo.m_tokenRanges;
    info.m_refusedByFilter = preInfo.m_refusedByFilter;

    // We do not compare result name and request for categorial requests but we prefer named
    // features.
    if (m_params.IsCategorialRequest())
    {
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

      auto nameScore = scores.first.m_nameScore;
      auto errorsMade = scores.first.m_errorsMade;
      bool isAltOrOldName = scores.first.m_isAltOrOldName;
      auto matchedLength = scores.second;

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

          nameScore = min(nameScore, streetScores.first.m_nameScore);
          errorsMade += streetScores.first.m_errorsMade;
          if (streetScores.first.m_isAltOrOldName)
            isAltOrOldName = true;
          matchedLength += streetScores.second;
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

      size_t totalLength = 0;
      for (size_t i = 0; i < m_params.GetNumTokens(); ++i)
        totalLength += m_params.GetToken(i).GetOriginal().size();

      info.m_nameScore = nameScore;
      info.m_errorsMade = errorsMade;
      info.m_isAltOrOldName = isAltOrOldName;
      info.m_matchedFraction =
          totalLength == 0 ? 1.0
                           : static_cast<double>(matchedLength) / static_cast<double>(totalLength);

      auto const isCountryOrCapital = [](FeatureType & ft) {
        auto static const countryType = classif().GetTypeByPath({"place", "country"});
        auto static const capitalType = classif().GetTypeByPath({"place", "city", "capital", "2"});

        bool hasType = false;
        ft.ForEachType([&hasType](uint32_t type) {
          if (hasType)
            return;
          if (type == countryType || type == capitalType)
            hasType = true;
        });

        return hasType;
      };
      info.m_exactCountryOrCapital = info.m_errorsMade == ErrorsMade(0) && info.m_allTokensUsed &&
                                     info.m_nameScore == NAME_SCORE_FULL_MATCH &&
                                     isCountryOrCapital(ft);
    }

    CategoriesInfo const categoriesInfo(feature::TypesHolder(ft),
                                        TokenSlice(m_params, preInfo.InnermostTokenRange()),
                                        m_ranker.m_params.m_categoryLocales, m_ranker.m_categories);

    info.m_pureCats = categoriesInfo.IsPureCategories();
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
    }
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
  SetLocale("default");
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

Result Ranker::MakeResult(RankerResult const & rankerResult, bool needAddress,
                          bool needHighlighting) const
{
  string name = rankerResult.GetName();

  string address;
  if (needAddress)
  {
    LazyAddressGetter addressGetter(m_reverseGeocoder, rankerResult.GetCenter());

    address = GetLocalizedRegionInfoForResult(rankerResult);

    // Format full address only for suitable results.
    if (ftypes::IsAddressObjectChecker::Instance()(rankerResult.GetTypes()))
      address = FormatFullAddress(addressGetter.GetNearbyAddress(), address);
  }

  // todo(@m) Used because Result does not have a default constructor. Factor out?
  auto mk = [&](RankerResult const & r) -> Result {
    switch (r.GetResultType())
    {
    case RankerResult::Type::Feature:
    case RankerResult::Type::Building:
    {
      auto const type = rankerResult.GetBestType(m_params.m_preferredTypes);
      return Result(r.GetID(), r.GetCenter(), name, address, type, r.GetDetails());
    }
    case RankerResult::Type::LatLon: return Result(r.GetCenter(), name, address);
    case RankerResult::Type::Postcode: return Result(r.GetCenter(), name);
    }
    ASSERT(false, ("Bad RankerResult type:", static_cast<size_t>(r.GetResultType())));
    UNREACHABLE();
  };

  auto res = mk(rankerResult);

  if (needAddress &&
      ftypes::IsLocalityChecker::Instance().GetType(rankerResult.GetTypes()) == ftypes::LocalityType::None)
  {
    m_localities.GetLocality(res.GetFeatureCenter(), [&](LocalityItem const & item) {
      string city;
      if (item.GetReadableName(city))
        res.PrependCity(city);
    });
  }

  if (needHighlighting)
    HighlightResult(m_params.m_tokens, m_params.m_prefix, res);

  res.SetRankingInfo(rankerResult.GetRankingInfo());
  res.SetProvenance(rankerResult.GetProvenance());
  return res;
}

void Ranker::SuggestStrings()
{
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
    // *NOTE* GetLinearModelRank is calculated on the fly
    // but the model is lightweight enough and the slowdown
    // is negligible.
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

    auto const & rankerResult = m_tentativeResults[i];

    if (count >= m_params.m_limit)
      break;

    Result result = MakeResult(rankerResult, m_params.m_needAddress, m_params.m_needHighlighting);

    if (m_params.m_viewportSearch)
    {
      m_emitter.AddResultNoChecks(move(result));
      ++count;
    }
    else
    {
      LOG(LDEBUG, (rankerResult));
      if (m_emitter.AddResult(move(result)))
        ++count;
    }
  }
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

    if (!ResultExists(*p, m_tentativeResults, m_params.m_minDistanceBetweenResultsM))
      m_tentativeResults.push_back(move(*p));
  };

  m_preRankerResults.clear();
}

void Ranker::GetBestMatchName(FeatureType & f, string & name) const
{
  int8_t bestLang = StringUtf8Multilang::kUnsupportedLanguageCode;
  KeywordLangMatcher::Score bestScore;
  auto updateScore = [&](int8_t lang, string const & s, bool force) {
    // Ignore name for categorial requests.
    auto const score = m_keywordsScorer.CalcScore(lang, m_params.m_categorialRequest ? "" : s);
    if (force ? bestScore <= score : bestScore < score)
    {
      bestScore = score;
      name = s;
      bestLang = lang;
    }
  };

  auto bestNameFinder = [&](int8_t lang, string const & s) {
    if (lang == StringUtf8Multilang::kAltNameCode || lang == StringUtf8Multilang::kOldNameCode)
    {
      auto const names = strings::Tokenize(s, ";");
      for (auto const & n : names)
        updateScore(lang, n, true /* force */);
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
    string readableName;
    f.GetReadableName(readableName);
    // Do nothing if alt/old name is the only name we have.
    if (readableName != name && !readableName.empty())
      name = readableName + " (" + name + ")";
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

void Ranker::ProcessSuggestions(vector<RankerResult> & vec) const
{
  if (m_params.m_prefix.empty() || !m_params.m_suggestsEnabled)
    return;

  size_t added = 0;
  for (auto i = vec.begin(); i != vec.end();)
  {
    RankerResult const & r = *i;

    ftypes::LocalityType const type = GetLocalityIndex(r.GetTypes());
    if (type == ftypes::LocalityType::Country || type == ftypes::LocalityType::City || r.IsStreet())
    {
      string suggestion;
      GetSuggestion(r, m_params.m_query, m_params.m_tokens, m_params.m_prefix, suggestion);
      if (!suggestion.empty() && added < kMaxNumSuggests)
      {
        // todo(@m) RankingInfo is lost here. Should it be?
        if (m_emitter.AddResult(Result(
                MakeResult(r, false /* needAddress */, true /* needHighlighting */), suggestion)))
        {
          ++added;
        }

        i = vec.erase(i);
        continue;
      }
    }
    ++i;
  }
}

string Ranker::GetLocalizedRegionInfoForResult(RankerResult const & result) const
{
  auto const type = result.GetBestType(m_params.m_preferredTypes);

  storage::CountryId id;
  if (!result.GetCountryId(m_infoGetter, type, id))
    return {};

  return m_regionInfoGetter.GetLocalizedFullName(id);
}
}  // namespace search
