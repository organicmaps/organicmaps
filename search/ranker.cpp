#include "search/ranker.hpp"

#include "search/emitter.hpp"
#include "search/string_intersection.hpp"
#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "indexer/feature_algo.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"
#include "std/unique_ptr.hpp"

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
  bestScores.m_nameScore = std::max(bestScores.m_nameScore, GetNameScore(name, slice));
  bestScores.m_errorsMade = ErrorsMade::Min(bestScores.m_errorsMade, GetErrorsMade(name, slice));
}

template <typename TSlice>
void UpdateNameScores(vector<strings::UniString> const & tokens, TSlice const & slice,
                     NameScores & bestScores)
{
  bestScores.m_nameScore = std::max(bestScores.m_nameScore, GetNameScore(tokens, slice));
  bestScores.m_errorsMade = ErrorsMade::Min(bestScores.m_errorsMade, GetErrorsMade(tokens, slice));
}

NameScores GetNameScores(FeatureType const & ft, Geocoder::Params const & params,
                         TokenRange const & range, Model::Type type)
{
  NameScores bestScores;

  TokenSlice slice(params, range);
  TokenSliceNoCategories sliceNoCategories(params, range);

  for (auto const & lang : params.GetLangs())
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

  return bestScores;
}

void RemoveDuplicatingLinear(vector<IndexedValue> & values)
{
  PreResult2::LessLinearTypesF lessCmp;
  PreResult2::EqualLinearTypesF equalCmp;

  sort(values.begin(), values.end(), [&lessCmp](IndexedValue const & lhs, IndexedValue const & rhs)
       {
         return lessCmp(*lhs, *rhs);
       });

  values.erase(unique(values.begin(), values.end(),
                      [&equalCmp](IndexedValue const & lhs, IndexedValue const & rhs)
                      {
                        return equalCmp(*lhs, *rhs);
                      }),
               values.end());
}

// Chops off the last query token (the "prefix" one) from |str| and stores the result in |res|.
void GetStringPrefix(string const & str, string & res)
{
  search::Delimiters delims;
  // Find start iterator of prefix in input query.
  using TIter = utf8::unchecked::iterator<string::const_iterator>;
  TIter iter(str.end());
  while (iter.base() != str.begin())
  {
    TIter prev = iter;
    --prev;

    if (delims(*prev))
      break;

    iter = prev;
  }

  // Assign result with input string without prefix.
  res.assign(str.begin(), iter.base());
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
}

/// Makes continuous range for tokens and prefix.
template <class TIter, class TValue>
class CombinedIter
{
  TIter m_cur;
  TIter m_end;
  TValue const * m_val;

public:
  CombinedIter(TIter cur, TIter end, TValue const * val) : m_cur(cur), m_end(end), m_val(val) {}

  TValue const & operator*() const
  {
    ASSERT(m_val != 0 || m_cur != m_end, ("dereferencing of empty iterator"));
    if (m_cur != m_end)
      return *m_cur;

    return *m_val;
  }

  CombinedIter & operator++()
  {
    if (m_cur != m_end)
      ++m_cur;
    else
      m_val = 0;
    return *this;
  }

  bool operator==(CombinedIter const & other) const
  {
    return m_val == other.m_val && m_cur == other.m_cur;
  }

  bool operator!=(CombinedIter const & other) const
  {
    return m_val != other.m_val || m_cur != other.m_cur;
  }
};
}  // namespace

class PreResult2Maker
{
  Ranker & m_ranker;
  Index const & m_index;
  Geocoder::Params const & m_params;
  storage::CountryInfoGetter const & m_infoGetter;

  unique_ptr<Index::FeaturesLoaderGuard> m_loader;

  bool LoadFeature(FeatureID const & id, FeatureType & ft)
  {
    if (!m_loader || m_loader->GetId() != id.m_mwmId)
      m_loader = make_unique<Index::FeaturesLoaderGuard>(m_index, id.m_mwmId);
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

  void InitRankingInfo(FeatureType const & ft, m2::PointD const & center, PreResult1 const & res,
                       search::RankingInfo & info)
  {
    auto const & preInfo = res.GetInfo();

    auto const & pivot = m_ranker.m_params.m_accuratePivotCenter;

    info.m_distanceToPivot = MercatorBounds::DistanceOnEarth(center, pivot);
    info.m_rank = preInfo.m_rank;
    info.m_type = preInfo.m_type;

    auto const nameScores = GetNameScores(ft, m_params, preInfo.InnermostTokenRange(), info.m_type);

    auto nameScore = nameScores.m_nameScore;
    auto errorsMade = nameScores.m_errorsMade;

    if (info.m_type != Model::TYPE_STREET &&
        preInfo.m_geoParts.m_street != IntersectionResult::kInvalidId)
    {
      auto const & mwmId = ft.GetID().m_mwmId;
      FeatureType street;
      if (LoadFeature(FeatureID(mwmId, preInfo.m_geoParts.m_street), street))
      {
        auto const nameScores = GetNameScores(
            street, m_params, preInfo.m_tokenRange[Model::TYPE_STREET], Model::TYPE_STREET);
        nameScore = min(nameScore, nameScores.m_nameScore);
        errorsMade += nameScores.m_errorsMade;
      }
    }

    info.m_nameScore = nameScore;
    info.m_errorsMade = errorsMade;

    TokenSlice slice(m_params, preInfo.InnermostTokenRange());
    feature::TypesHolder holder(ft);
    vector<pair<size_t, size_t>> matched(slice.Size());
    ForEachCategoryType(QuerySlice(slice), m_ranker.m_params.m_categoryLocales,
                        m_ranker.m_categories, [&](size_t i, uint32_t t)
                        {
                          ++matched[i].second;
                          if (holder.Has(t))
                            ++matched[i].first;
                        });

    info.m_pureCats = all_of(matched.begin(), matched.end(), [](pair<size_t, size_t> const & m)
                             {
                               return m.first != 0;
                             });
    info.m_falseCats = all_of(matched.begin(), matched.end(), [](pair<size_t, size_t> const & m)
                              {
                                return m.first == 0 && m.second != 0;
                              });
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

    // For all other search types, rank should be zero for now.
    default: return 0;
    }
  }

public:
  explicit PreResult2Maker(Ranker & ranker, Index const & index,
                           storage::CountryInfoGetter const & infoGetter,
                           Geocoder::Params const & params)
    : m_ranker(ranker), m_index(index), m_params(params), m_infoGetter(infoGetter)
  {
  }

  unique_ptr<PreResult2> operator()(PreResult1 const & res1)
  {
    FeatureType ft;
    m2::PointD center;
    string name;
    string country;

    if (!LoadFeature(res1.GetId(), ft, center, name, country))
      return unique_ptr<PreResult2>();

    auto res2 = make_unique<PreResult2>(ft, center, m_ranker.m_params.m_position /* pivot */, name,
                                        country);

    search::RankingInfo info;
    InitRankingInfo(ft, center, res1, info);
    info.m_rank = NormalizeRank(info.m_rank, info.m_type, center, country);
    res2->SetRankingInfo(move(info));

    return res2;
  }
};

// static
size_t const Ranker::kBatchSize = 10;

Ranker::Ranker(Index const & index, CitiesBoundariesTable const & boundariesTable,
               storage::CountryInfoGetter const & infoGetter, Emitter & emitter,
               CategoriesHolder const & categories, vector<Suggest> const & suggests,
               VillagesCache & villagesCache, my::Cancellable const & cancellable)
  : m_reverseGeocoder(index)
  , m_cancellable(cancellable)
  , m_localities(index, boundariesTable, villagesCache)
  , m_index(index)
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
  m_preResults1.clear();
  m_tentativeResults.clear();
}

bool Ranker::IsResultExists(PreResult2 const & p, vector<IndexedValue> const & values)
{
  PreResult2::StrictEqualF equalCmp(p, m_params.m_minDistanceOnMapBetweenResults);

  // Do not insert duplicating results.
  return values.end() != find_if(values.begin(), values.end(), [&equalCmp](IndexedValue const & iv)
                                 {
                                   return equalCmp(*iv);
                                 });
}

void Ranker::MakePreResult2(Geocoder::Params const & geocoderParams, vector<IndexedValue> & cont)
{
  PreResult2Maker maker(*this, m_index, m_infoGetter, geocoderParams);
  for (auto const & r : m_preResults1)
  {
    auto p = maker(r);
    if (!p)
      continue;

    if (geocoderParams.m_mode == Mode::Viewport &&
        !geocoderParams.m_pivot.IsPointInside(p->GetCenter()))
    {
      continue;
    }

    if (!IsResultExists(*p, cont))
      cont.push_back(IndexedValue(move(p)));
  };
}

Result Ranker::MakeResult(PreResult2 const & r) const
{
  Result res = r.GenerateFinalResult(m_infoGetter, &m_categories, &m_params.m_preferredTypes,
                                     m_params.m_currentLocaleCode, &m_reverseGeocoder);
  MakeResultHighlight(res);
  if (ftypes::IsLocalityChecker::Instance().GetType(r.GetTypes()) == ftypes::NONE)
  {
    m_localities.GetLocality(res.GetFeatureCenter(), [&](LocalityItem const & item) {
      string city;
      if (item.GetSpecifiedOrDefaultName(m_localityLang, city))
        res.AppendCity(city);
    });
  }

  res.SetRankingInfo(r.GetRankingInfo());
  return res;
}

void Ranker::MakeResultHighlight(Result & res) const
{
  using TIter = buffer_vector<strings::UniString, 32>::const_iterator;
  using TCombinedIter = CombinedIter<TIter, strings::UniString>;

  TCombinedIter beg(m_params.m_tokens.begin(), m_params.m_tokens.end(),
                    m_params.m_prefix.empty() ? 0 : &m_params.m_prefix);
  TCombinedIter end(m_params.m_tokens.end(), m_params.m_tokens.end(), 0);
  auto assignHighlightRange = [&](pair<uint16_t, uint16_t> const & range)
  {
    res.AddHighlightRange(range);
  };

  SearchStringTokensIntersectionRanges(res.GetString(), beg, end, assignHighlightRange);
}

void Ranker::GetSuggestion(string const & name, string & suggest) const
{
  // Splits result's name.
  search::Delimiters delims;
  vector<strings::UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), delims);

  // Finds tokens that are already present in the input query.
  vector<bool> tokensMatched(tokens.size());
  bool prefixMatched = false;
  bool fullPrefixMatched = false;

  for (size_t i = 0; i < tokens.size(); ++i)
  {
    auto const & token = tokens[i];

    if (find(m_params.m_tokens.begin(), m_params.m_tokens.end(), token) != m_params.m_tokens.end())
    {
      tokensMatched[i] = true;
    }
    else if (StartsWith(token, m_params.m_prefix))
    {
      prefixMatched = true;
      fullPrefixMatched = token.size() == m_params.m_prefix.size();
    }
  }

  // When |name| does not match prefix or when prefix equals to some
  // token of the |name| (for example, when user entered "Moscow"
  // without space at the end), we should not suggest anything.
  if (!prefixMatched || fullPrefixMatched)
    return;

  GetStringPrefix(m_params.m_query, suggest);

  // Appends unmatched result's tokens to the suggestion.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    if (tokensMatched[i])
      continue;
    suggest.append(strings::ToUtf8(tokens[i]));
    suggest.push_back(' ');
  }
}

void Ranker::SuggestStrings()
{
  if (m_params.m_prefix.empty() || !m_params.m_suggestsEnabled)
    return;

  string prologue;
  GetStringPrefix(m_params.m_query, prologue);

  for (auto const & locale : m_params.m_categoryLocales)
    MatchForSuggestions(m_params.m_prefix, locale, prologue);
}

void Ranker::MatchForSuggestions(strings::UniString const & token, int8_t locale,
                                 string const & prologue)
{
  for (auto const & suggest : m_suggests)
  {
    strings::UniString const & s = suggest.m_name;
    if ((suggest.m_prefixLength <= token.size()) &&
        (token != s) &&                  // do not push suggestion if it already equals to token
        (suggest.m_locale == locale) &&  // push suggestions only for needed language
        strings::StartsWith(s.begin(), s.end(), token.begin(), token.end()))
    {
      string const utf8Str = strings::ToUtf8(s);
      Result r(utf8Str, prologue + utf8Str + " ");
      MakeResultHighlight(r);
      m_emitter.AddResult(move(r));
    }
  }
}

void Ranker::GetBestMatchName(FeatureType const & f, string & name) const
{
  KeywordLangMatcher::ScoreT bestScore;
  auto bestNameFinder = [&](int8_t lang, string const & s) -> bool
  {
    auto const score = m_keywordsScorer.Score(lang, s);
    if (bestScore < score)
    {
      bestScore = score;
      name = s;
    }
    return true;
  };
  UNUSED_VALUE(f.ForEachName(bestNameFinder));
}

void Ranker::ProcessSuggestions(vector<IndexedValue> & vec) const
{
  if (m_params.m_prefix.empty() || !m_params.m_suggestsEnabled)
    return;

  int added = 0;
  for (auto i = vec.begin(); i != vec.end();)
  {
    PreResult2 const & r = **i;

    ftypes::Type const type = GetLocalityIndex(r.GetTypes());
    if ((type == ftypes::COUNTRY || type == ftypes::CITY) || r.IsStreet())
    {
      string suggest;
      GetSuggestion(r.GetName(), suggest);
      if (!suggest.empty() && added < MAX_SUGGESTS_COUNT)
      {
        if (m_emitter.AddResult(Result(MakeResult(r), suggest)))
          ++added;

        i = vec.erase(i);
        continue;
      }
    }
    ++i;
  }
}

void Ranker::UpdateResults(bool lastUpdate)
{
  BailIfCancelled();

  MakePreResult2(m_geocoderParams, m_tentativeResults);
  RemoveDuplicatingLinear(m_tentativeResults);
  if (m_tentativeResults.empty())
    return;

  if (m_params.m_viewportSearch)
  {
    sort(m_tentativeResults.begin(), m_tentativeResults.end(),
         my::LessBy(&IndexedValue::GetDistanceToPivot));
  }
  else
  {
    sort(m_tentativeResults.rbegin(), m_tentativeResults.rend(),
         my::LessBy(&IndexedValue::GetRank));
    ProcessSuggestions(m_tentativeResults);
  }

  // Emit feature results.
  size_t count = m_emitter.GetResults().GetCount();
  size_t i = 0;
  for (; i < m_tentativeResults.size(); ++i)
  {
    if (!lastUpdate && i >= kBatchSize && !m_params.m_viewportSearch)
      break;
    BailIfCancelled();

    if (m_params.m_viewportSearch)
    {
      m_emitter.AddResultNoChecks(
          (*m_tentativeResults[i])
              .GenerateFinalResult(m_infoGetter, &m_categories, &m_params.m_preferredTypes,
                                   m_params.m_currentLocaleCode,
                                   nullptr /* Viewport results don't need calculated address */));
    }
    else
    {
      if (count >= m_params.m_limit)
        break;

      LOG(LDEBUG, (m_tentativeResults[i]));

      auto const & preResult2 = *m_tentativeResults[i];
      if (m_emitter.AddResult(MakeResult(preResult2)))
        ++count;
    }
  }
  m_tentativeResults.erase(m_tentativeResults.begin(), m_tentativeResults.begin() + i);

  m_preResults1.clear();

  BailIfCancelled();
  m_emitter.Emit();
}

void Ranker::ClearCaches()
{
  m_localities.ClearCache();
}
}  // namespace search
