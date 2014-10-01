#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"

namespace df
{

class LineShape : public MapShape
{
public:
  LineShape(vector<m2::PointD> const & points,
            LineViewParams const & params,
            float const scaleGtoP);

  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

  float         GetWidth() const { return m_params.m_width; }
  dp::Color const & GetColor() const { return m_params.m_color; }

private:
  void doPartition(uint32_t patternLength, uint32_t templateLength, vector<m2::PointF> & points) const;
  bool GetNext(m2::PointF & point) const;

private:
  LineViewParams m_params;
  vector<m2::PointD> m_dpoints;
  float const m_scaleGtoP;
  mutable int m_counter;
  mutable int m_parts;
  mutable uint32_t m_patternLength;
  mutable uint32_t m_templateLength;
};

} // namespace df

