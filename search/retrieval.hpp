#pragma once

#include "search/search_query_params.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"

#include "std/function.hpp"
#include "std/vector.hpp"

class Index;

namespace search
{
class Retrieval
{
public:
  class Callback
  {
  public:
    virtual ~Callback() = default;

    virtual void OnMwmProcessed(MwmSet::MwmId const & id, vector<uint32_t> const & offsets) = 0;
  };

  // Following class represents a set of retrieval's limits.
  struct Limits
  {
  public:
    Limits();

    // Sets lower bound on number of features to be retrieved.
    void SetMinNumFeatures(uint64_t minNumFeatures);
    uint64_t GetMinNumFeatures() const;

    // Sets upper bound on a maximum viewport's scale.
    void SetMaxViewportScale(double maxViewportScale);
    double GetMaxViewportScale() const;

    inline bool IsMinNumFeaturesSet() const { return m_minNumFeaturesSet; }
    inline bool IsMaxViewportScaleSet() const { return m_maxViewportScaleSet; }

  private:
    uint64_t m_minNumFeatures;
    double m_maxViewportScale;

    bool m_minNumFeaturesSet : 1;
    bool m_maxViewportScaleSet : 1;
  };

  Retrieval();

  void Init(Index & index, m2::RectD const & viewport, SearchQueryParams const & params,
            Limits const & limits);

  void Go(Callback & callback);

private:
  struct FeatureBucket
  {
    FeatureBucket(MwmSet::MwmHandle && handle);

    MwmSet::MwmHandle m_handle;
    vector<uint32_t> m_addressFeatures;
    vector<uint32_t> m_geometryFeatures;
    vector<uint32_t> m_intersection;
    m2::RectD m_bounds;
    bool m_intersectsWithViewport : 1;
    bool m_coveredByViewport : 1;
    bool m_finished : 1;
  };

  // *NOTE* arguments of successive calls of this method should be
  // *non-decreasing.
  void RetrieveForViewport(m2::RectD const & viewport, Callback & callback);

  bool ViewportCoversAllMwms() const;

  uint64_t CountRetrievedFeatures() const;

  Index * m_index;
  m2::RectD m_viewport;
  SearchQueryParams m_params;
  Limits m_limits;

  vector<FeatureBucket> m_buckets;
};
}  // namespace search
