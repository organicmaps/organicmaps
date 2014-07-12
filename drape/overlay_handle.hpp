#pragma once

#include "binding_info.hpp"
#include "index_buffer_mutator.hpp"
#include "attribute_buffer_mutator.hpp"

#include "../indexer/feature_decl.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"


class OverlayHandle
{
public:
  enum Anchor
  {
    Center      = 0,
    Left        = 0x1,
    Right       = Left << 1,
    Top         = Right << 1,
    Bottom      = Top << 1,
    LeftTop     = Left | Top,
    RightTop    = Right | Top,
    LeftBottom  = Left | Bottom,
    RightBottom = Right | Bottom
  };

  OverlayHandle(FeatureID const & id,
                Anchor anchor,
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
  Anchor const m_anchor;
  double const m_priority;

  typedef pair<BindingInfo, MutateRegion> offset_node_t;
  offset_node_t const & GetOffsetNode(uint8_t bufferID) const;

private:
  bool m_isVisible;

  vector<uint16_t> m_indexes;
  struct LessOffsetNode
  {
    bool operator()(offset_node_t const & node1, offset_node_t const & node2) const
    {
      return node1.first.GetID() < node2.first.GetID();
    }
  };

  struct OffsetNodeFinder;

  set<offset_node_t, LessOffsetNode> m_offsets;
};

class SquareHandle : public OverlayHandle
{
  typedef OverlayHandle base_t;
public:
  SquareHandle(FeatureID const & id,
               Anchor anchor,
               m2::PointD const & gbPivot,
               m2::PointD const & pxSize,
               double priority);

  m2::RectD GetPixelRect(ScreenBase const & screen) const;

private:
  m2::PointD m_gbPivot;
  m2::PointD m_pxHalfSize;
};
