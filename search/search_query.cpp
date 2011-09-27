#include "search_query.hpp"
#include "categories_holder.hpp"
#include "feature_offset_match.hpp"
#include "latlon_match.hpp"
#include "result.hpp"
#include "../indexer/feature_covering.hpp"
#include "../indexer/features_vector.hpp"
#include "../indexer/index.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/search_delimiters.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../base/logging.hpp"
#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"
#include "../std/algorithm.hpp"

namespace search
{

Query::Query(Index const * pIndex, search::CategoriesHolder const * pCategories)
  : m_pIndex(pIndex), m_pCategories(pCategories), m_viewport(m2::RectD::GetEmptyRect()),
    m_viewportExtended(m2::RectD::GetEmptyRect())
{
}

Query::~Query()
{
}

void Query::SetViewport(m2::RectD const & viewport)
{
  // TODO: Clear m_viewportExtended when mwm index is added or removed!

  if (m_viewport != viewport)
  {
    m_viewport = viewport;
    m_viewportExtended = m_viewport;
    m_viewportExtended.Scale(3);

    UpdateViewportOffsets();
  }
}

void Query::UpdateViewportOffsets()
{
  vector<MwmInfo> mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);
  m_offsetsInViewport.resize(mwmInfo.size());

  int const viewScale = scales::GetScaleLevel(m_viewport);
  covering::CoveringGetter cov(m_viewport, 0);

  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (m_viewportExtended.IsIntersect(mwmInfo[mwmId].m_limitRect))
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      if (MwmValue * pMwm = mwmLock.GetValue())
      {
        feature::DataHeader const & header = pMwm->GetHeader();
        if (header.GetType() == feature::DataHeader::worldcoasts)
          continue;

        pair<int, int> const scaleR = header.GetScaleRange();
        int const scale = min(max(viewScale + 7, scaleR.first), scaleR.second);

        covering::IntervalsT const & interval = cov.Get(scaleR);

        ScaleIndex<ModelReaderPtr> index(pMwm->m_cont.GetReader(INDEX_FILE_TAG),
                                         pMwm->m_factory);

        for (size_t i = 0; i < interval.size(); ++i)
        {
          index.ForEachInIntervalAndScale(MakeInsertFunctor(m_offsetsInViewport[mwmId]),
                                          interval[i].first, interval[i].second,
                                          scale);
        }
      }
    }
  }

  size_t offsetsCached = 0;
  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
    offsetsCached += m_offsetsInViewport[mwmId].size();
  LOG(LINFO, ("For search in viewport cached ",
              "mwms:", mwmInfo.size(),
              "offsets:", offsetsCached));
}

void Query::Search(string const & query,
                   function<void (Result const &)> const & f,
                   unsigned int resultsNeeded)
{
  // Initialize.
  {
    m_rawQuery = query;
    m_uniQuery = NormalizeAndSimplifyString(m_rawQuery);

    search::Delimiters delims;
    SplitUniString(m_uniQuery, MakeBackInsertFunctor(m_tokens), delims);
    if (!m_tokens.empty() && !delims(strings::LastUniChar(m_rawQuery)))
    {
      m_prefix.swap(m_tokens.back());
      m_tokens.pop_back();
    }
    if (m_tokens.size() > 31)
      m_tokens.resize(31);

    m_results = my::limited_priority_queue<impl::IntermediateResult>(resultsNeeded);
  }

  // Match (lat, lon).
  {
    double lat, lon, latPrec, lonPrec;
    if (search::MatchLatLon(m_rawQuery, lat, lon, latPrec, lonPrec))
    {
      double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
      AddResult(impl::IntermediateResult(m_viewport, lat, lon, precision));
    }
  }

  SearchFeatures();

  FlushResults(f);
}

void Query::AddResult(impl::IntermediateResult const & result)
{
  m_results.push(result);
}

void Query::FlushResults(function<void (Result const &)> const & f)
{
  vector<impl::IntermediateResult> v(m_results.begin(), m_results.end());
  sort_heap(v.begin(), v.end());
  for (vector<impl::IntermediateResult>::const_iterator it = v.begin(); it != v.end(); ++it)
    f(it->GenerateFinalResult());
}

namespace impl
{

struct FeatureLoader
{
  uint32_t m_count;
  FeaturesVector & m_featuresVector;
  Query & m_query;

  FeatureLoader(FeaturesVector & featuresVector, Query & query)
    : m_count(0), m_featuresVector(featuresVector), m_query(query)
  {
  }

  void operator() (uint32_t offset)
  {
    ++m_count;
    FeatureType feature;
    m_featuresVector.Get(offset, feature);
    m_query.AddResult(impl::IntermediateResult(m_query.m_viewport, feature));
  }
};

}  // namespace search::impl

void Query::SearchFeatures()
{
  if (!m_pIndex)
    return;

  vector<MwmInfo> mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);

  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (m_viewportExtended.IsIntersect(mwmInfo[mwmId].m_limitRect))
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      if (MwmValue * pMwm = mwmLock.GetValue())
      {
        if (pMwm->m_cont.IsReaderExist(SEARCH_INDEX_FILE_TAG))
        {
          scoped_ptr<TrieIterator> pTrieRoot(::trie::reader::ReadTrie(
                                               pMwm->m_cont.GetReader(SEARCH_INDEX_FILE_TAG),
                                               ::search::trie::ValueReader(),
                                               ::search::trie::EdgeValueReader()));
          if (pTrieRoot)
          {
            FeaturesVector featuresVector(pMwm->m_cont, pMwm->GetHeader());
            impl::FeatureLoader f(featuresVector, *this);
            MatchFeaturesInTrie(m_tokens.data(), m_tokens.size(), m_prefix, *pTrieRoot,
                                &m_offsetsInViewport[mwmId], f, m_results.max_size() * 10);

            LOG(LINFO, ("Matched: ", f.m_count));
          }
        }
      }
    }
  }
}

}  // namespace search
