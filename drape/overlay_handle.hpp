#pragma once

#include "drape_global.hpp"
#include "binding_info.hpp"
#include "index_buffer_mutator.hpp"
#include "attribute_buffer_mutator.hpp"

#include "../indexer/feature_decl.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

namespace dp
{

class OverlayHandle
{
public:
  OverlayHandle(FeatureID const & id,
                dp::Anchor anchor,
                double priority);

  virtual ~OverlayHandle() {}

  bool IsVisible() const;
  void SetIsVisible(bool isVisible);

  virtual void Update(ScreenBase const & /*screen*/) {}
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const = 0;
  uint16_t * IndexStorage(uint16_t size);
  size_t GetIndexCount() const;
  void GetElementIndexes(RefPointer<IndexBufferMutator> mutator) const;
  virtual void GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const;

  bool HasDynamicAttributes() const;
  void AddDynamicAttribute(BindingInfo const & binding, uint16_t offset, uint16_t count);

  FeatureID const & GetFeatureID() const;
  double const & GetPriority() const;

protected:
  FeatureID const m_id;
  dp::Anchor const m_anchor;
  double const m_priority;

  typedef pair<BindingInfo, MutateRegion> TOffsetNode;
  TOffsetNode const & GetOffsetNode(uint8_t bufferID) const;

private:
  bool m_isVisible;

  vector<uint16_t> m_indexes;
  struct LessOffsetNode
  {
    bool operator()(TOffsetNode const & node1, TOffsetNode const & node2) const
    {
      return node1.first.GetID() < node2.first.GetID();
    }
  };

  struct OffsetNodeFinder;

  set<TOffsetNode, LessOffsetNode> m_offsets;
};

class SquareHandle : public OverlayHandle
{
  typedef OverlayHandle base_t;
public:
  SquareHandle(FeatureID const & id,
               dp::Anchor anchor,
               m2::PointD const & gbPivot,
               m2::PointD const & pxSize,
               double priority);

  m2::RectD GetPixelRect(ScreenBase const & screen) const;

private:
  m2::PointD m_gbPivot;
  m2::PointD m_pxHalfSize;
};

} // namespace dp
