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
#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/preferred_languages.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <memory>

#include <boost/optional.hpp>

using namespace std;

namespace search
{
namespace
{
struct NameScores
{
  NameScore m_nameScore = NAME_SCORE_ZERO;
  ErrorsMade m_errorsMade;
};

template <typename TSlice>
void UpdateNameScores(string const & name, TSlice const & slice, NameScores & bestScores)
{
  bestScores.m_nameScore = max(bestScores.m_nameScore, GetNameScore(name, slice));
  bestScores.m_errorsMade = ErrorsMade::Min(bestScores.m_errorsMade, GetErrorsMade(name, slice));
}

template <typename TSlice>
void UpdateNameScores(vector<strings::UniString> const & tokens, TSlice const & slice,
                      NameScores & bestScores)
{
  bestScores.m_nameScore = max(bestScores.m_nameScore, GetNameScore(tokens, slice));
  bestScores.m_errorsMade = ErrorsMade::Min(bestScores.m_errorsMade, GetErrorsMade(tokens, slice));
}

NameScores GetNameScores(FeatureType & ft, Geocoder::Params const & params,
                         TokenRange const & range, Model::Type type)
{
  NameScores bestScores;

  TokenSlice slice(params, range);
  TokenSliceNoCategories sliceNoCategories(params, range);

  for (auto const lang : params.GetLangs())
  {
    string name;
    if (!ft.GetName(lang, name))
      continue;
    vector<strings::UniString> tokens;
    PrepareStringForMatching(name, tokens);

    UpdateNameScores(tokens, slice, bestScores);
    UpdateNameScores(tokens, sliceNoCategories, bestScores);
  }

  if (type == Model::TYPE_BUILDING)
    UpdateNameScores(ft.GetHouseNumber(), sliceNoCategories, bestScores);

  if (ftypes::IsAirportChecker::Instance()(ft))
  {
    string const iata = ft.GetMetadata().Get(feature::Metadata::FMD_AIRPORT_IATA);
    if (!iata.empty())
      UpdateNameScores(iata, sliceNoCategories, bestScores);
  }

  string const op = ft.GetMetadata().Get(feature::Metadata::FMD_OPERATOR);
  if (!op.empty())
    UpdateNameScores(op, sliceNoCategories, bestScores);

  string const brand = ft.GetMetadata().Get(feature::Metadata::FMD_BRAND);
  if (!brand.empty())
  {
    auto const & brands = indexer::GetDefaultBrands();
    brands.ForEachNameByKey(brand, [&](indexer::BrandsHolder::Brand::Name const & name) {
      UpdateNameScores(name.m_name, sliceNoCategories, bestScores);
    });
  }

  return bestScores;
}

ErrorsMade GetErrorsMade(FeatureType & ft, Geocoder::Params const & params,
                         TokenRange const & range, Model::Type type)
{
  auto errorsMade = GetNameScores(ft, params, range, type).m_errorsMade;
  if (errorsMade.IsValid())
    return errorsMade;

  for (auto const token : range)
  {
    ErrorsMade tokenErrors;
    params.GetToken(token).ForEach([&](strings::UniString const & s) {
      tokenErrors = ErrorsMade::Max(tokenErrors, ErrorsMade{GetMaxErrorsForToken(s)});
    });
    errorsMade += tokenErrors;
  }

  return errorsMade;
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
    return r1.GetDistance() < r2.GetDistance();
  };

  auto equalCmp = [](RankerResult const & r1, RankerResult const & r2) -> bool {
    // Note! Do compare for distance when filtering linear objects.
    // Otherwise we will skip the results for different parts of the map.
    return r1.GetGeomType() == feature::GEOM_LINE && r1.IsEqualCommon(r2) &&
           PointDistance(r1.GetCenter(), r2.GetCenter()) < kDistSameStreetMeters;
  };

  sort(results.begin(), results.end(), lessCmp);
  results.erase(unique(results.begin(), results.end(), equalCmp), results.end());
}

ftypes::Type GetLocalityIndex(feature::TypesHolder const & types)
{
  using namespace ftypes;

  // Inner logic of SearchAddress expects COUNTRY, STATE and CITY only.
  Type const type = IsLocalityChecker::Instance().GetType(types);
  switch (type)
  {
  case NONE:
  case COUNTRY:
  case STATE:
  case CITY: return type;
  case TOWN: return CITY;
  case VILLAGE: return NONE;
  case LOCALITY_COUNT: return type;
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
        p.GetResultType() == RankerResult::Type::TYPE_FEATURE)
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
  Ranker & m_ranker;
  DataSource const & m_dataSource;
  Geocoder::Params const & m_params;
  storage::CountryInfoGetter const & m_infoGetter;

  unique_ptr<FeaturesLoaderGuard> m_loader;

  bool LoadFeature(FeatureID const & id, FeatureType & ft)
  {
    if (!m_loader || m_loader->GetId() != id.m_mwmId)
      m_loader = make_unique<FeaturesLoaderGuard>(m_dataSource, id.m_mwmId);
    if (!m_loader->GetFeatureByIndex(id.m_index, ft))
      return false;

    ft.SetID(id);
    return true;
  }

  // For the best performance, incoming ids should be sorted by id.first (mwm file id).
  bool LoadFeature(FeatureID const & id, FeatureType & ft, m2::PointD & center, string & name,
                   string & country)
  {
    if (!LoadFeature(id, ft))
      return false;

    center = feature::GetCenter(ft);
    m_ranker.GetBestMatchName(ft, name);

    // Country (region) name is a file name if feature isn't from
    // World.mwm.
    ASSERT(m_loader && m_loader->GetId() == id.m_mwmId, ());
    if (m_loader->IsWorld())
      country.clear();
    else
      country = m_loader->GetCountryFileName();

    return true;
  }

  void InitRankingInfo(FeatureType & ft, m2::PointD const & center, PreRankerResult const & res,
                       search::RankingInfo & info)
  {
    auto const & preInfo = res.GetInfo();

    auto const & pivot = m_ranker.m_params.m_accuratePivotCenter;

    info.m_distanceToPivot = MercatorBounds::DistanceOnEarth(center, pivot);
    info.m_rank = preInfo.m_rank;
    info.m_popularity = preInfo.m_popularity;
    info.m_type = preInfo.m_type;
    info.m_allTokensUsed = preInfo.m_allTokensUsed;
    info.m_categorialRequest = m_params.IsCategorialRequest();
    info.m_hasName = ft.HasName();

    // We do not compare result name and request for categorial requests.
    if (!m_params.IsCategorialRequest())
    {
      auto const nameScores =
          GetNameScores(ft, m_params, preInfo.InnermostTokenRange(), info.m_type);

      auto nameScore = nameScores.m_nameScore;
      auto errorsMade = nameScores.m_errorsMade;

      if (info.m_type != Model::TYPE_STREET &&
          preInfo.m_geoParts.m_street != IntersectionResult::kInvalidId)
      {
        auto const & mwmId = ft.GetID().m_mwmId;
        FeatureType street;
        if (LoadFeature(FeatureID(mwmId, preInfo.m_geoParts.m_street), street))
        {
          auto const type = Model::TYPE_STREET;
          auto const & range = preInfo.m_tokenRange[type];
          auto const nameScores = GetNameScores(street, m_params, range, type);

          nameScore = min(nameScore, nameScores.m_nameScore);
          errorsMade += nameScores.m_errorsMade;
        }
      }

      if (!Model::IsLocalityType(info.m_type) && preInfo.m_cityId.IsValid())
      {
        FeatureType city;
        if (LoadFeature(preInfo.m_cityId, city))
        {
          auto const type = Model::TYPE_CITY;
          auto const & range = preInfo.m_tokenRange[type];
          errorsMade += GetErrorsMade(city, m_params, range, type);
        }
      }

      info.m_nameScore = nameScore;
      info.m_errorsMade = errorsMade;
    }

    CategoriesInfo const categoriesInfo(feature::TypesHolder(ft),
                                        TokenSlice(m_params, preInfo.InnermostTokenRange()),
                                        m_ranker.m_params.m_categoryLocales, m_ranker.m_categories);

    info.m_pureCats = categoriesInfo.IsPureCategories();
    info.m_falseCats = categoriesInfo.IsFalseCategories();
  }

  uint8_t NormalizeRank(uint8_t rank, Model::Type type, m2::PointD const & center,
                        string const & country)
  {
    switch (type)
    {
    case Model::TYPE_VILLAGE: return rank /= 1.5;
    case Model::TYPE_CITY:
    {
      if (m_ranker.m_params.m_viewport.IsPointInside(center))
        return rank * 2;

      storage::CountryInfo info;
      if (country.empty())
        m_infoGetter.GetRegionInfo(center, info);
      else
        m_infoGetter.GetRegionInfo(country, info);
      if (info.IsNotEmpty() && info.m_name == m_ranker.m_params.m_pivotRegion)
        return rank *= 1.7;
    }
    case Model::TYPE_COUNTRY:
      return rank /= 1.5;

    default: return rank;
    }
  }

public:
  RankerResultMaker(Ranker & ranker, DataSource const & dataSource,
                    storage::CountryInfoGetter const & infoGetter, Geocoder::Params const & params)
    : m_ranker(ranker), m_dataSource(dataSource), m_params(params), m_infoGetter(infoGetter)
  {
  }

  boost::optional<RankerResult> operator()(PreRankerResult const & preRankerResult)
  {
    FeatureType ft;
    m2::PointD center;
    string name;
    string country;

    if (!LoadFeature(preRankerResult.GetId(), ft, center, name, country))
      return {};

    RankerResult r(ft, center, m_ranker.m_params.m_pivot, name, country);

    search::RankingInfo info;
    InitRankingInfo(ft, center, preRankerResult, info);
    info.m_rank = NormalizeRank(info.m_rank, info.m_type, center, country);
    r.SetRankingInfo(move(info));

    return r;
  }
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
  // The results have been updated by PreRanker.
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

    // Insert exact address (street and house number) instead of empty result name.
    if (name.empty())
    {
      ReverseGeocoder::Address addr;
      if (addressGetter.GetExactAddress(addr))
        name = FormatStreetAndHouse(addr);
    }

    address = GetLocalizedRegionInfoForResult(rankerResult);

    // Format full address only for suitable results.
    if (ftypes::IsAddressObjectChecker::Instance()(rankerResult.GetTypes()))
      address = FormatFullAddress(addressGetter.GetNearbyAddress(), address);
  }

  // todo(@m) Used because Result does not have a default constructor. Factor out?
  auto mk = [&](RankerResult const & r) -> Result {
    switch (r.GetResultType())
    {
    case RankerResult::Type::TYPE_FEATURE:
    case RankerResult::Type::TYPE_BUILDING:
    {
      auto const type = rankerResult.GetBestType(m_params.m_preferredTypes);
      return Result(r.GetID(), r.GetCenter(), name, address, type, r.GetMetadata());
    }
    case RankerResult::Type::TYPE_LATLON: return Result(r.GetCenter(), name, address);
    }
    ASSERT(false, ("Bad RankerResult type:", static_cast<size_t>(r.GetResultType())));
    UNREACHABLE();
  };

  auto res = mk(rankerResult);

  if (needAddress &&
      ftypes::IsLocalityChecker::Instance().GetType(rankerResult.GetTypes()) == ftypes::NONE)
  {
    m_localities.GetLocality(res.GetFeatureCenter(), [&](LocalityItem const & item) {
      string city;
      if (item.GetSpecifiedOrDefaultName(m_localeCode, city))
        res.PrependCity(city);
    });
  }

  if (needHighlighting)
    HighlightResult(m_params.m_tokens, m_params.m_prefix, res);

  res.SetRankingInfo(rankerResult.GetRankingInfo());
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

  MakeRankerResults(m_geocoderParams, m_tentativeResults);
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
      if (count % m_params.m_batchSize == 0)
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

  m_preRankerResults.clear();

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
  m_localeCode = StringUtf8Multilang::GetLangIndex(languages::Normalize(locale));
  m_regionInfoGetter.SetLocale(locale);
}

void Ranker::LoadCountriesTree() { m_regionInfoGetter.LoadCountriesTree(); }

void Ranker::MakeRankerResults(Geocoder::Params const & geocoderParams,
                               vector<RankerResult> & results)
{
  RankerResultMaker maker(*this, m_dataSource, m_infoGetter, geocoderParams);
  for (auto const & r : m_preRankerResults)
  {
    auto p = maker(r);
    if (!p)
      continue;

    if (geocoderParams.m_mode == Mode::Viewport &&
        !geocoderParams.m_pivot.IsPointInside(p->GetCenter()))
    {
      continue;
    }

    if (!ResultExists(*p, results, m_params.m_minDistanceBetweenResultsM))
      results.push_back(move(*p));
  };
}

void Ranker::GetBestMatchName(FeatureType & f, string & name) const
{
  KeywordLangMatcher::Score bestScore;
  auto updateScore = [&](int8_t lang, string const & s, bool force) {
    auto const score = m_keywordsScorer.CalcScore(lang, s);
    if (force ? bestScore <= score : bestScore < score)
    {
      bestScore = score;
      name = s;
    }
  };

  auto bestNameFinder = [&](int8_t lang, string const & s) {
    updateScore(lang, s, true /* force */);
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

    ftypes::Type const type = GetLocalityIndex(r.GetTypes());
    if (type == ftypes::COUNTRY || type == ftypes::CITY || r.IsStreet())
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

  storage::TCountryId id;
  if (!result.GetCountryId(m_infoGetter, type, id))
    return {};

  return m_regionInfoGetter.GetLocalizedFullName(id);
}
}  // namespace search
