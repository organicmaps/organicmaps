#pragma once

#include "drape/feature_geometry_decl.hpp"
#include "drape/pointers.hpp"

#include "std/function.hpp"

class ScreenBase;

namespace df
{
class BatchMergeHelper;
}

namespace dp
{

class OverlayHandle;
class OverlayTree;
class VertexArrayBuffer;

class RenderBucket
{
  friend class df::BatchMergeHelper;
public:
  RenderBucket(drape_ptr<VertexArrayBuffer> && buffer);
  ~RenderBucket();

  ref_ptr<VertexArrayBuffer> GetBuffer();
  drape_ptr<VertexArrayBuffer> && MoveBuffer();

  size_t GetOverlayHandlesCount() const;
  drape_ptr<OverlayHandle> PopOverlayHandle();
  ref_ptr<OverlayHandle> GetOverlayHandle(size_t index);
  void AddOverlayHandle(drape_ptr<OverlayHandle> && handle);

  void Update(ScreenBase const & modelView);
  void CollectOverlayHandles(ref_ptr<OverlayTree> tree);
  void Render(ScreenBase const & screen);

  // Only for testing! Don't use this function in production code!
  void RenderDebug(ScreenBase const & screen) const;

  // Only for testing! Don't use this function in production code!
  template <typename ToDo>
  void ForEachOverlay(ToDo const & todo)
  {
    for (drape_ptr<OverlayHandle> const & h : m_overlay)
      todo(make_ref(h));
  }

  void StartFeatureRecord(FeatureGeometryId feature, m2::RectD const & limitRect);
  void EndFeatureRecord(bool featureCompleted);

  using TCheckFeaturesWaiting = function<bool(m2::RectD const &)>;
  bool IsFeaturesWaiting(TCheckFeaturesWaiting isFeaturesWaiting);

private:
  struct FeatureGeometryInfo
  {
    FeatureGeometryInfo(m2::RectD const & limitRect)
      : m_limitRect(limitRect)
    {}

    m2::RectD m_limitRect;
    bool m_featureCompleted = true;
  };
  using TFeaturesRanges = map<FeatureGeometryId, FeatureGeometryInfo>;

  FeatureGeometryId m_featureInfo;
  m2::RectD m_featureLimitRect;
  TFeaturesRanges m_featuresRanges;

  vector<drape_ptr<OverlayHandle> > m_overlay;
  drape_ptr<VertexArrayBuffer> m_buffer;
};

} // namespace dp
