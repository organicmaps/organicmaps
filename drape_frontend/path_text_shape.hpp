#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../drape/overlay_handle.hpp"

#include "../std/vector.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/spline.hpp"

namespace df
{

using m2::PointF;

struct LetterInfo
{
  LetterInfo(float xOff, float yOff, float adv, float hw, float hh)
  : m_xOffset(xOff), m_yOffset(yOff), m_advance(adv),
    m_halfWidth(hw), m_halfHeight(hh){}

  LetterInfo(){}

  float m_xOffset;
  float m_yOffset;
  float m_advance;
  float m_halfWidth;
  float m_halfHeight;
};

namespace
{
struct Position
{
  Position() {}
  Position(float x, float y) : m_x(x), m_y(y){}
  Position(PointF const & p) : m_x(p.x), m_y(p.y){}

  float m_x;
  float m_y;
};
}

class PathTextShape : public MapShape
{
public:
  PathTextShape(vector<PointF> const & path, PathTextViewParams const & params);
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  PathTextViewParams m_params;
  m2::Spline m_path;
};

class PathTextHandle : public dp::OverlayHandle
{
public:
  static const uint8_t DirectionAttributeID = 1;
  PathTextHandle(m2::Spline const & spl, PathTextViewParams const & params, vector<LetterInfo> const & info, float maxSize)
    : OverlayHandle(FeatureID(), dp::Center, 0.0f),
      m_params(params), m_path(spl), m_infos(info),
      m_scaleFactor(1.0f), m_positions(info.size() * 6),
      m_maxSize(maxSize) {}

  virtual void Update(ScreenBase const & screen);
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const;
  virtual void GetAttributeMutation(dp::RefPointer<dp::AttributeBufferMutator> mutator) const;

private:
  PathTextViewParams m_params;
  m2::Spline m_path;
  vector<LetterInfo> m_infos;
  float m_scaleFactor;
  mutable vector<Position> m_positions;
  float m_maxSize;
};

} // namespace df
