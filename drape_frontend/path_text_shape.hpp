#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../drape/overlay_handle.hpp"

#include "../std/vector.hpp"

#include "../geometry/point2d.hpp"

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

class Spline
{
public:
  Spline(){}
  void FromArray(vector<PointF> const & path);
  Spline const & operator = (Spline const & spl);

public:
  float m_lengthAll;
  vector<PointF> m_position;
  vector<PointF> m_direction;
  vector<float> m_length;

  class iterator
  {
  public:
    int m_index;
    PointF m_pos;
    PointF m_dir;
    iterator()
      : m_spl(NULL), m_index(0), m_dist(0),
        m_pos(PointF()), m_dir(PointF()) {}
    void Attach(Spline const & S);
    void Step(float speed);
  private:
    Spline const * m_spl;
    float m_dist;
  };
};

class PathTextShape : public MapShape
{
public:
  PathTextShape(vector<PointF> const & path, PathTextViewParams const & params);
  virtual void Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const;

private:
  PathTextViewParams m_params;
  Spline m_path;
};

class PathTextHandle : public OverlayHandle
{
public:
  static const uint8_t DirectionAttributeID = 1;
  PathTextHandle(Spline const & spl, PathTextViewParams const & params, vector<LetterInfo> const & info)
    : OverlayHandle(FeatureID(), dp::Center, 0.0f),
      m_params(params), m_path(spl), m_infos(info)
  {
    SetIsVisible(true);
    m_scaleFactor = 1.0f;
  }

  virtual void Update(ScreenBase const & screen)
  {
    m_scaleFactor = screen.GetScale();
  }
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const { return m2::RectD(); }

  virtual void GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const;

private:
  PathTextViewParams m_params;
  Spline m_path;
  vector<LetterInfo> m_infos;
  float m_scaleFactor;
};

} // namespace df
