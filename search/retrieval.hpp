#pragma once

#include "search/search_query_params.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/macros.hpp"

#include "std/function.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class Index;

namespace search
{
class Retrieval : public my::Cancellable
{
public:
  class Callback
  {
  public:
    virtual ~Callback() = default;

    // Called each time a bunch of features for an mwm is retrieved.
    // This method may be called several times for the same mwm,
    // reporting disjoint sets of features.
    virtual void OnFeaturesRetrieved(MwmSet::MwmId const & id, double scale,
                                     vector<uint32_t> const & featureIds) = 0;
  };

  // This class wraps a set of retrieval's limits like number of
  // features to be retrieved, maximum viewport scale, etc.
  struct Limits
  {
  public:
    Limits();

    // Sets upper bound (inclusive) on a number of features to be
    // retrieved.
    void SetMaxNumFeatures(uint64_t minNumFeatures);
    uint64_t GetMaxNumFeatures() const;
    inline bool IsMaxNumFeaturesSet() const { return m_maxNumFeaturesSet; }

    // Sets upper bound on a maximum viewport's scale.
    void SetMaxViewportScale(double maxViewportScale);
    double GetMaxViewportScale() const;
    inline bool IsMaxViewportScaleSet() const { return m_maxViewportScaleSet; }

    // Sets whether retrieval should/should not skip World.mwm.
    inline void SetSearchInWorld(bool searchInWorld) { m_searchInWorld = searchInWorld; }
    inline bool GetSearchInWorld() const { return m_searchInWorld; }

  private:
    uint64_t m_maxNumFeatures;
    double m_maxViewportScale;

    bool m_maxNumFeaturesSet : 1;
    bool m_maxViewportScaleSet : 1;
    bool m_searchInWorld : 1;
  };

  // This class represents a retrieval's strategy.
  class Strategy
  {
  public:
    using TCallback = function<void(vector<uint32_t> &)>;

    Strategy(MwmSet::MwmHandle & handle, m2::RectD const & viewport);

    virtual ~Strategy() = default;

    // Retrieves features for m_viewport scaled by |scale|. Returns
    // false when cancelled.
    //
    // *NOTE* This method should be called on a strictly increasing
    // *sequence of scales.
    WARN_UNUSED_RESULT bool Retrieve(double scale, my::Cancellable const & cancellable,
                                     TCallback const & callback);

   protected:
     WARN_UNUSED_RESULT virtual bool RetrieveImpl(double scale, my::Cancellable const & cancellable,
                                                  TCallback const & callback) = 0;

    MwmSet::MwmHandle & m_handle;
    m2::RectD const m_viewport;
    double m_prevScale;
  };

  Retrieval();

  void Init(Index & index, vector<shared_ptr<MwmInfo>> const & infos, m2::RectD const & viewport,
            SearchQueryParams const & params, Limits const & limits);

  // Start retrieval process.
  //
  // *NOTE* Retrieval may report features not belonging to viewport
  // (even scaled by maximum allowed scale). The reason is the current
  // geomerty index algorithm - when it asked for features in a
  // rectangle, it reports all features from cells that cover (not
  // covered by) a rectangle.
  void Go(Callback & callback);

private:
  // This class is a wrapper around single mwm during retrieval
  // process.
  struct Bucket
  {
    Bucket(MwmSet::MwmHandle && handle);

    MwmSet::MwmHandle m_handle;
    m2::RectD m_bounds;
    vector<uint32_t> m_addressFeatures;

    // The order matters here - strategy may contain references to the
    // fields above, thus it must be destructed before them.
    unique_ptr<Strategy> m_strategy;

    size_t m_featuresReported;
    bool m_intersectsWithViewport : 1;
    bool m_finished : 1;
  };

  // Retrieves features for the viewport scaled by |scale| and invokes
  // callback on retrieved features. Returns false when cancelled.
  //
  // *NOTE* |scale| of successive calls of this method should be
  // non-decreasing.
  WARN_UNUSED_RESULT bool RetrieveForScale(double scale, Callback & callback);

  // Returns true when all buckets are marked as finished.
  bool Finished() const;

  // Reports features, updates bucket's stats.
  void ReportFeatures(Bucket & bucket, vector<uint32_t> & featureIds, double scale,
                      Callback & callback);

  Index * m_index;
  m2::RectD m_viewport;
  SearchQueryParams m_params;
  Limits m_limits;
  uint64_t m_featuresReported;

  vector<Bucket> m_buckets;
};
}  // namespace search
