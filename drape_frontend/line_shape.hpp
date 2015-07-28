#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/binding_info.hpp"
#include "drape/glstate.hpp"

#include "geometry/spline.hpp"

#include "std/unique_ptr.hpp"

namespace df
{

class LineBuilder
{
public:
  virtual ~LineBuilder() {}

  virtual dp::BindingInfo const & GetBindingInfo() = 0;
  virtual dp::GLState GetState() = 0;

  virtual ref_ptr<void> GetLineData() = 0;
  virtual size_t GetLineSize() = 0;

  virtual ref_ptr<void> GetJoinData() = 0;
  virtual size_t GetJoinSize() = 0;
};

class LineShape : public MapShape
{
public:
  LineShape(m2::SharedSpline const & spline,
            LineViewParams const & params,
            ref_ptr<dp::TextureManager> textures);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;

private:
  template <typename TBuilder>
  void Construct(TBuilder & builder) const;

  LineViewParams m_params;
  m2::SharedSpline m_spline;
  unique_ptr<LineBuilder> m_lineBuilder;
};

} // namespace df

