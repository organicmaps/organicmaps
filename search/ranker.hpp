#pragma once

#include "search/geocoder.hpp"
#include "search/intermediate_result.hpp"
#include "search/mode.hpp"
#include "search/processor.hpp"

#include "indexer/feature_decl.hpp"

#include "std/vector.hpp"

namespace search
{
class PreResult2Maker;
class Processor;

class Ranker
{
public:
  Ranker(PreRanker & preRanker, Processor & processor)
    : m_viewportSearch(false), m_preRanker(preRanker), m_processor(processor)
  {
  }

  void Init(bool viewportSearch) { m_viewportSearch = viewportSearch; }

  bool IsResultExists(PreResult2 const & p, vector<IndexedValue> const & indV);

  void MakePreResult2(Geocoder::Params const & params, vector<IndexedValue> & cont,
                      vector<FeatureID> & streets);

  Result MakeResult(PreResult2 const & r) const;

  void FlushResults(Geocoder::Params const & params, Results & res, size_t resCount);
  void FlushViewportResults(Geocoder::Params const & params, Results & res);

private:
  bool m_viewportSearch;
  PreRanker & m_preRanker;

  // todo(@m) Remove.
  Processor & m_processor;
};

}  // namespace search
