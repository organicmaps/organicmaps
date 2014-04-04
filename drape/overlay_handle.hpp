#pragma once

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "index_buffer_mutator.hpp"

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

  OverlayHandle(Anchor anchor, m2::PointD const & gbPivot,
                m2::PointD const & pxSize);

  bool IsVisible() const;
  void SetIsVisible(bool isVisible);

  m2::RectD GetPixelRect(ScreenBase const & screen) const;
  uint16_t * IndexStorage(uint16_t size);
  size_t GetIndexCount() const;
  void GetElementIndexes(RefPointer<IndexBufferMutator> mutator) const;

private:
  Anchor m_anchor;
  m2::PointD m_gbPivot;
  m2::PointD m_pxHalfSize;
  bool m_isVisible;

  vector<uint16_t> m_indexes;
};
