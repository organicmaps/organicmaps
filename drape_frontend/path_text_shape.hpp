#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../std/vector.hpp"

#include "../geometry/point2d.hpp"

namespace df
{

using m2::PointF;

class Spline
{
public:
  float length_all;
  vector<PointF> position;
  vector<PointF> direction;
  vector<float> length;
  int amount_ind;

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
        index %= spl->amount_ind;
      }
      dir = spl->direction[index];
      pos = spl->position[index] + dir * dist;
    }
  };
};

class PathTextShape : public MapShape
{
public:
  PathTextShape(vector<PointF> const & path, TextViewParams const & params);
  virtual void Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const;

  void Load(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const;
  void update(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const;

private:
  TextViewParams m_params;
  Spline m_path;
};

} // namespace df
