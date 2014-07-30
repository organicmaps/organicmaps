#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"
#include "path_text_shape.hpp"

#include "../drape/overlay_handle.hpp"

#include "../std/vector.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/spline.hpp"

namespace df
{

class PathSymbolShape : public MapShape
{
public:
  PathSymbolShape(vector<m2::PointF> const & path, PathSymbolViewParams const & params, float maxScale);
  virtual void Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const;

private:
  PathSymbolViewParams m_params;
  m2::Spline m_path;
  float m_maxScale;
};

class PathSymbolHandle : public OverlayHandle
{
public:
  static const uint8_t PositionAttributeID = 1;
  PathSymbolHandle(m2::Spline const & spl, PathSymbolViewParams const & params, int maxCount, float hw, float hh)
    : OverlayHandle(FeatureID(), dp::Center, 0.0f),
      m_params(params), m_path(spl), m_scaleFactor(1.0f),
      m_positions(maxCount * 6), m_maxCount(maxCount),
      m_symbolHalfWidth(hw), m_symbolHalfHeight(hh) {}

  virtual void Update(ScreenBase const & screen);
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const;
  virtual void GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const;

private:
  PathSymbolViewParams m_params;
  m2::Spline m_path;
  float m_scaleFactor;
  mutable vector<Position> m_positions;
  int m_maxCount;
  float m_symbolHalfWidth;
  float m_symbolHalfHeight;
};

}
