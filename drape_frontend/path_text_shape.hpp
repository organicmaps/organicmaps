#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"
#include "common_structures.hpp"

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

class PathTextShape : public MapShape
{
public:
  PathTextShape(vector<PointF> const & path, PathTextViewParams const & params);
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

private:
  m2::SharedSpline m_spline;
  PathTextViewParams m_params;
};

class PathTextHandle : public dp::OverlayHandle
{
public:
  static const uint8_t DirectionAttributeID = 1;
  PathTextHandle(m2::SharedSpline const & spl, PathTextViewParams const & params,
                 vector<LetterInfo> const & info, float maxSize, float textLength);

  virtual void Update(ScreenBase const & screen);
  void DrawReverse(ScreenBase const & screen);
  void DrawForward(ScreenBase const & screen);
  void ClearPositions();
  int ChooseDirection(ScreenBase const & screen);
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const;
  virtual void GetAttributeMutation(dp::RefPointer<dp::AttributeBufferMutator> mutator) const;

private:
  m2::SharedSpline m_path;
  PathTextViewParams m_params;
  vector<LetterInfo> m_infos;
  float m_scaleFactor;
  mutable vector<glsl_types::vec2> m_positions;
  float m_maxSize;
  float m_textLength;
};

} // namespace df
