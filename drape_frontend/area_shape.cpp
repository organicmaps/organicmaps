#include "area_shape.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"

namespace df
{
  AreaShape::AreaShape(const Color & c, float depth)
    : m_color(c)
    , m_depth(depth)
  {}

  void AreaShape::AddTriangle(const m2::PointF & v1,
                              const m2::PointF & v2,
                              const m2::PointF & v3)
  {
    m_vertexes.push_back(Point3D(v1.x, v1.y, m_depth));
    m_vertexes.push_back(Point3D(v2.x, v2.y, m_depth));
    m_vertexes.push_back(Point3D(v3.x, v3.y, m_depth));
  }

  void AreaShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureManager> /*textures*/) const
  {
    GLState state(gpu::SOLID_AREA_PROGRAM, 0);
    float r, g, b, a;
    ::Convert(m_color, r, g, b, a);
    state.GetUniformValues().SetFloatValue("color", r, g, b, a);

    AttributeProvider provider(1, m_vertexes.size());
    {
      BindingInfo info(1);
      BindingDecl & decl = info.GetBindingDecl(0);
      decl.m_attributeName = "position";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(0, info, MakeStackRefPointer((void *)&m_vertexes[0]));
    }

    batcher->InsertTriangleList(state, MakeStackRefPointer(&provider));
  }
}
