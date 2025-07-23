#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/binding_info.hpp"

#include "geometry/spline.hpp"

#include <memory>

namespace df
{
class LineShapeInfo
{
public:
  virtual ~LineShapeInfo() = default;

  virtual dp::BindingInfo const & GetBindingInfo() = 0;
  virtual dp::RenderState GetState() = 0;

  virtual ref_ptr<void> GetLineData() = 0;
  virtual uint32_t GetLineSize() = 0;

  //  virtual ref_ptr<void> GetJoinData() = 0;
  //  virtual uint32_t GetJoinSize() = 0;

  virtual dp::BindingInfo const & GetCapBindingInfo() = 0;
  virtual dp::RenderState GetCapState() = 0;
  virtual ref_ptr<void> GetCapData() = 0;
  virtual uint32_t GetCapSize() = 0;
};

class LineShape : public MapShape
{
public:
  LineShape(m2::SharedSpline const & spline, LineViewParams const & params);

  void Prepare(ref_ptr<dp::TextureManager> textures) const override;
  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;

private:
  glsl::vec2 ToShapeVertex2(m2::PointD const & vertex) const
  {
    return glsl::ToVec2(ConvertToLocal(vertex, m_params.m_tileCenter, kShapeCoordScalar));
  }

  template <class FnT>
  void ForEachSplineSection(FnT && fn) const;

  template <typename TBuilder>
  void Construct(TBuilder & builder) const;

  bool CanBeSimplified(int & lineWidth) const;

  LineViewParams m_params;
  m2::SharedSpline m_spline;
  mutable std::unique_ptr<LineShapeInfo> m_lineShapeInfo;
  mutable bool m_isSimple;
};
}  // namespace df
