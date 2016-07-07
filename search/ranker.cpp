#include "search/ranker.hpp"
#include "search/pre_ranker.hpp"
#include "search/string_intersection.hpp"
#include "search/token_slice.hpp"
#include "search/utils.hpp"

#include "indexer/feature_algo.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace
{
template <typename TSlice>
void UpdateNameScore(string const & name, TSlice const & slice, NameScore & bestScore)
{
  auto const score = GetNameScore(name, slice);
  if (score > bestScore)
    bestScore = score;
}

template <typename TSlice>
void UpdateNameScore(vector<strings::UniString> const & tokens, TSlice const & slice,
                     NameScore & bestScore)
{
  auto const score = GetNameScore(tokens, slice);
  if (score > bestScore)
    bestScore = score;
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

  unique_ptr<Index::FeaturesLoaderGuard> m_pFV;

  // For the best performance, incoming id's should be sorted by id.first (mwm file id).
  bool LoadFeature(FeatureID const & id, FeatureType & f, m2::PointD & center, string & name,
                   string & country)
  {
    if (m_pFV.get() == 0 || m_pFV->GetId() != id.m_mwmId)
      m_pFV.reset(new Index::FeaturesLoaderGuard(m_index, id.m_mwmId));

    if (!m_pFV->GetFeatureByIndex(id.m_index, f))
      return false;

    f.SetID(id);

    center = feature::GetCenter(f);

    m_ranker.GetBestMatchName(f, name);

    // country (region) name is a file name if feature isn't from World.mwm
    if (m_pFV->IsWorld())
      country.clear();
    else
      country = m_pFV->GetCountryFileName();

    return true;
  }

  void InitRankingInfo(FeatureType const & ft, m2::PointD const & center, PreResult1 const & res,
                       search::RankingInfo & info)
  {
    auto const & preInfo = res.GetInfo();

    auto const & pivot = m_params.m_accuratePivotCenter;

    info.m_distanceToPivot = MercatorBounds::DistanceOnEarth(center, pivot);
    info.m_rank = preInfo.m_rank;
    info.m_searchType = preInfo.m_searchType;
    info.m_nameScore = NAME_SCORE_ZERO;

    TokenSlice slice(m_params, preInfo.m_startToken, preInfo.m_endToken);
    TokenSliceNoCategories sliceNoCategories(m_params, preInfo.m_startToken, preInfo.m_endToken);

    for (auto const & lang : m_params.m_langs)
    {
      string name;
      if (!ft.GetName(lang, name))
        continue;
      vector<strings::UniString> tokens;
      SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), Delimiters());

      UpdateNameScore(tokens, slice, info.m_nameScore);
      UpdateNameScore(tokens, sliceNoCategories, info.m_nameScore);
    }

    if (info.m_searchType == SearchModel::SEARCH_TYPE_BUILDING)
      UpdateNameScore(ft.GetHouseNumber(), sliceNoCategories, info.m_nameScore);

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

  uint8_t NormalizeRank(uint8_t rank, SearchModel::SearchType type, m2::PointD const & center,
                        string const & country)
  {
    switch (type)
    {
    case SearchModel::SEARCH_TYPE_VILLAGE: return rank /= 1.5;
    case SearchModel::SEARCH_TYPE_CITY:
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
    case SearchModel::SEARCH_TYPE_COUNTRY:
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

    auto res2 = make_unique<PreResult2>(ft, &res1, center, m_ranker.m_params.m_position /* pivot */,
                                        name, country);

    search::RankingInfo info;
    InitRankingInfo(ft, center, res1, info);
    info.m_rank = NormalizeRank(info.m_rank, info.m_searchType, center, country);
    res2->SetRankingInfo(move(info));

    return res2;
  }
};

bool Ranker::IsResultExists(PreResult2 const & p, vector<IndexedValue> const & values)
{
  PreResult2::StrictEqualF equalCmp(p);
  // Do not insert duplicating results.
  return values.end() != find_if(values.begin(), values.end(), [&equalCmp](IndexedValue const & iv)
                                 {
                                   return equalCmp(*iv);
                                 });
}

void Ranker::MakePreResult2(Geocoder::Params const & geocoderParams, vector<IndexedValue> & cont,
                            vector<FeatureID> & streets)
{
  m_preRanker.Filter(m_params.m_viewportSearch);

  // Makes PreResult2 vector.
  PreResult2Maker maker(*this, m_index, m_infoGetter, geocoderParams);
  m_preRanker.ForEach(
      [&](PreResult1 const & r)
      {
        auto p = maker(r);
        if (!p)
          return;

        if (geocoderParams.m_mode == Mode::Viewport &&
            !geocoderParams.m_pivot.IsPointInside(p->GetCenter()))
          return;

        if (p->IsStreet())
          streets.push_back(p->GetID());

        if (!IsResultExists(*p, cont))
          cont.push_back(IndexedValue(move(p)));
      });
}

Result Ranker::MakeResult(PreResult2 const & r) const
{
  Result res = r.GenerateFinalResult(m_infoGetter, &m_categories, &m_params.m_preferredTypes,
                                     m_params.m_currentLocaleCode, &m_reverseGeocoder);
  MakeResultHighlight(res);
#ifdef FIND_LOCALITY_TEST
  if (ftypes::IsLocalityChecker::Instance().GetType(r.GetTypes()) == ftypes::NONE)
  {
    string city;
    m_locality.GetLocality(res.GetFeatureCenter(), city);
    res.AppendCity(city);
  }
#endif  // FIND_LOCALITY_TEST

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

void Ranker::SuggestStrings(Results & res)
{
  if (m_params.m_prefix.empty() || !m_params.m_suggestsEnabled)
    return;

  string prologue;
  GetStringPrefix(m_params.m_query, prologue);

  for (int i = 0; i < m_params.m_categoryLocales.size(); ++i)
    MatchForSuggestions(m_params.m_prefix, m_params.m_categoryLocales[i], prologue, res);
}

void Ranker::MatchForSuggestions(strings::UniString const & token, int8_t locale,
                                 string const & prologue, Results & res)
{
  for (auto const & suggest : m_suggests)
  {
    strings::UniString const & s = suggest.m_name;
    if ((suggest.m_prefixLength <= token.size()) &&
        (token != s) &&                  // do not push suggestion if it already equals to token
        (suggest.m_locale == locale) &&  // push suggestions only for needed language
        StartsWith(s.begin(), s.end(), token.begin(), token.end()))
    {
      string const utf8Str = strings::ToUtf8(s);
      Result r(utf8Str, prologue + utf8Str + " ");
      MakeResultHighlight(r);
      res.AddResult(move(r));
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

void Ranker::ProcessSuggestions(vector<IndexedValue> & vec, Results & res) const
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
        if (res.AddResult((Result(MakeResult(r), suggest))))
          ++added;

        i = vec.erase(i);
        continue;
      }
    }
    ++i;
  }
}

void Ranker::FlushResults(Geocoder::Params const & params, Results & res, size_t resCount)
{
  vector<IndexedValue> values;
  vector<FeatureID> streets;

  MakePreResult2(params, values, streets);
  RemoveDuplicatingLinear(values);
  if (values.empty())
    return;

  sort(values.rbegin(), values.rend(), my::LessBy(&IndexedValue::GetRank));

  ProcessSuggestions(values, res);

  // Emit feature results.
  size_t count = res.GetCount();
  for (size_t i = 0; i < values.size() && count < resCount; ++i)
  {
    BailIfCancelled();

    LOG(LDEBUG, (values[i]));

    auto const & preResult2 = *values[i];
    if (res.AddResult(MakeResult(preResult2)))
      ++count;
  }
}

void Ranker::FlushViewportResults(Geocoder::Params const & geocoderParams, Results & res)
{
  vector<IndexedValue> values;
  vector<FeatureID> streets;

  MakePreResult2(geocoderParams, values, streets);
  RemoveDuplicatingLinear(values);
  if (values.empty())
    return;

  sort(values.begin(), values.end(), my::LessBy(&IndexedValue::GetDistanceToPivot));

  for (size_t i = 0; i < values.size(); ++i)
  {
    BailIfCancelled();

    res.AddResultNoChecks(
        (*(values[i]))
            .GenerateFinalResult(m_infoGetter, &m_categories, &m_params.m_preferredTypes,
                                 m_params.m_currentLocaleCode,
                                 nullptr /* Viewport results don't need calculated address */));
  }
}

void Ranker::ClearCaches()
{
#ifdef FIND_LOCALITY_TEST
  m_locality.ClearCache();
#endif  // FIND_LOCALITY_TEST
}
}  // namespace search
