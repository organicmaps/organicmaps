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
  LetterInfo(float xOff, float yOff, float adv, float hw, float hh):
    m_xOffset(xOff), m_yOffset(yOff), m_advance(adv),
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
  float length_all;
  vector<PointF> position;
  vector<PointF> direction;
  vector<float> length;

public:
  Spline(){}

  void FromArray(vector<PointF> const & path)
  {
    position = vector<PointF>(path.begin(), path.end() - 1);
    int cnt = position.size();
    direction = vector<PointF>(cnt);
    length = vector<float>(cnt);

    for(int i = 0; i < cnt; ++i)
    {
      direction[i] = path[i+1] - path[i];
      length[i] = direction[i].Length();
      direction[i] = direction[i].Normalize();
      length_all += length[i];
    }
  }

  Spline const & operator = (Spline const & spl)
  {
    if(&spl != this)
    {
      length_all = spl.length_all;
      position = spl.position;
      direction = spl.direction;
      length = spl.length;
    }
    return *this;
  }

  class iterator
  {
  private:
    Spline const * spl;
    float dist;
  public:
    int index;
    PointF pos;
    PointF dir;
    iterator(){}
    void Attach(Spline const & S)
    {
      spl = &S;
      index = 0;
      dist = 0;
      dir = spl->direction[index];
      pos = spl->position[index] + dir * dist;
    }
    void Step(float speed)
    {
      dist += speed;
      while(dist > spl->length[index])
      {
        dist -= spl->length[index];
        index++;
        index %= spl->position.size();
      }
      dir = spl->direction[index];
      pos = spl->position[index] + dir * dist;
    }
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
      m_params(params),
      m_path(spl),
      m_infos(info)
  {
    SetIsVisible(true);
    scaleFactor = 1.0f;
  }

  virtual void Update(ScreenBase const & screen)
  {
    scaleFactor = screen.GetScale();
  }
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const { return m2::RectD(); }

  virtual void GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const;

private:
  PathTextViewParams m_params;
  Spline m_path;
  vector<LetterInfo> m_infos;
  float scaleFactor;
};

} // namespace df
