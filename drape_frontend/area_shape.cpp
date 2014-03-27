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
    for_each(triangleList.begin(), triangleList.end(),
             bind(&vector<Point3D>::push_back, &m_vertexes,
                  bind(&Point3D::From2D, _1, params.m_depth)));
  }

  void AreaShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> /*textures*/) const
  {
    GLState state(gpu::SOLID_AREA_PROGRAM, 0);
    state.SetColor(m_params.m_color);

    AttributeProvider provider(1, m_vertexes.size());
    {
      BindingInfo info(1);
      BindingDecl & decl = info.GetBindingDecl(0);
      decl.m_attributeName = "a_position";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(0, info, MakeStackRefPointer((void *)&m_vertexes[0]));
    }

    batcher->InsertTriangleList(state, MakeStackRefPointer(&provider));
  }
}
