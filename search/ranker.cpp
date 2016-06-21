#include "search/ranker.hpp"
#include "search/processor.hpp"
#include "search/token_slice.hpp"

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
}  // namespace

class PreResult2Maker
{
  Processor & m_processor;
  Geocoder::Params const & m_params;

  unique_ptr<Index::FeaturesLoaderGuard> m_pFV;

  // For the best performance, incoming id's should be sorted by id.first (mwm file id).
  void LoadFeature(FeatureID const & id, FeatureType & f, m2::PointD & center, string & name,
                   string & country)
  {
    if (m_pFV.get() == 0 || m_pFV->GetId() != id.m_mwmId)
      m_pFV.reset(new Index::FeaturesLoaderGuard(m_processor.m_index, id.m_mwmId));

    m_pFV->GetFeatureByIndex(id.m_index, f);
    f.SetID(id);

    center = feature::GetCenter(f);

    m_processor.GetBestMatchName(f, name);

    // country (region) name is a file name if feature isn't from World.mwm
    if (m_pFV->IsWorld())
      country.clear();
    else
      country = m_pFV->GetCountryFileName();
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
    m_processor.ForEachCategoryType(QuerySlice(slice), [&](size_t i, uint32_t t)
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
      if (m_processor.GetViewport(Processor::CURRENT_V).IsPointInside(center))
        return rank * 2;

      storage::CountryInfo info;
      if (country.empty())
        m_processor.m_infoGetter.GetRegionInfo(center, info);
      else
        m_processor.m_infoGetter.GetRegionInfo(country, info);
      if (info.IsNotEmpty() && info.m_name == m_processor.GetPivotRegion())
        return rank *= 1.7;
    }
    case SearchModel::SEARCH_TYPE_COUNTRY:
      return rank /= 1.5;

    // For all other search types, rank should be zero for now.
    default: return 0;
    }
  }

public:
  explicit PreResult2Maker(Processor & q, Geocoder::Params const & params)
    : m_processor(q), m_params(params)
  {
  }

  unique_ptr<PreResult2> operator()(PreResult1 const & res1)
  {
    FeatureType ft;
    m2::PointD center;
    string name;
    string country;

    LoadFeature(res1.GetId(), ft, center, name, country);

    auto res2 = make_unique<PreResult2>(ft, &res1, center, m_processor.GetPosition() /* pivot */,
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

void Ranker::MakePreResult2(Geocoder::Params const & params, vector<IndexedValue> & cont,
                            vector<FeatureID> & streets)
{
  m_preRanker.Filter(m_viewportSearch);

  // Makes PreResult2 vector.
  PreResult2Maker maker(m_processor, params);
  m_preRanker.ForEach(
      [&](PreResult1 const & r)
      {
        auto p = maker(r);
        if (!p)
          return;

        if (params.m_mode == Mode::Viewport && !params.m_pivot.IsPointInside(p->GetCenter()))
          return;

        if (p->IsStreet())
          streets.push_back(p->GetID());

        if (!IsResultExists(*p, cont))
          cont.push_back(IndexedValue(move(p)));
      });
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

  m_processor.ProcessSuggestions(values, res);

  // Emit feature results.
  size_t count = res.GetCount();
  for (size_t i = 0; i < values.size() && count < resCount; ++i)
  {
    if (m_processor.IsCancelled())
      break;

    LOG(LDEBUG, (values[i]));

    auto const & preResult2 = *values[i];
    if (res.AddResult(m_processor.MakeResult(preResult2)))
      ++count;
  }
}

void Ranker::FlushViewportResults(Geocoder::Params const & params, Results & res)
{
  vector<IndexedValue> values;
  vector<FeatureID> streets;

  MakePreResult2(params, values, streets);
  RemoveDuplicatingLinear(values);
  if (values.empty())
    return;

  sort(values.begin(), values.end(), my::LessBy(&IndexedValue::GetDistanceToPivot));

  for (size_t i = 0; i < values.size(); ++i)
  {
    if (m_processor.IsCancelled())
      break;

    res.AddResultNoChecks(
        (*(values[i]))
            .GenerateFinalResult(m_processor.m_infoGetter, &m_processor.m_categories,
                                 &m_processor.m_prefferedTypes, m_processor.m_currentLocaleCode,
                                 nullptr /* Viewport results don't need calculated address */));
  }
}
}  // namespace search
