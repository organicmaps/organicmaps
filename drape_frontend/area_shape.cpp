#include "area_shape.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"

#include "../std/bind.hpp"

namespace df
{

AreaShape::AreaShape(vector<m2::PointF> const & triangleList, AreaViewParams const & params)
  : m_params(params)
{
  m_vertexes.reserve(triangleList.size());

#if (__cplusplus > 199711L) || defined(__GXX_EXPERIMENTAL_CXX0X__)
  void (vector<Point3D>::* fn)(Point3D &&) =
#else
  void (vector<Point3D>::* fn)(Point3D const &) =
#endif
      &vector<Point3D>::push_back;

  for_each(triangleList.begin(), triangleList.end(),
           bind(fn, &m_vertexes, bind(&Point3D::From2D, _1, params.m_depth)));
}

void AreaShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> /*textures*/) const
{
  dp::GLState state(gpu::SOLID_AREA_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColor(m_params.m_color);

  dp::AttributeProvider provider(1, m_vertexes.size());
  {
    dp::BindingInfo info(1);
    dp::BindingDecl & decl = info.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, info, dp::MakeStackRefPointer((void *)&m_vertexes[0]));
  }

  batcher->InsertTriangleList(state, dp::MakeStackRefPointer(&provider));
}

} // namespace df
