#include "line_shape.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"

namespace df
{
  LineShape::LineShape(const vector<m2::PointF> & points,
                       const Color & color,
                       float depth,
                       float width)
    : m_color(color)
    , m_depth(depth)
    , m_width(width)
  {
    ASSERT(!points.empty(), ());

    m_points.reserve(4*points.size());
    m_normals.reserve(4*points.size());

    const float hw = width/2;
    typedef m2::PointF vec2;

    vec2 start = points[0];
    for (size_t i = 1; i < points.size(); i++)
    {
      vec2 end = points[i];
      vec2 segment = end - start;

      if (segment.IsAlmostZero())
        continue;

      if (i < points.size() - 1)
      {
        vec2 longer = points[i+1] - start;
        const bool isCodirected = my::AlmostEqual(m2::DotProduct(segment, longer),
                                                  static_cast<float>(segment.Length()*longer.Length()));
        const bool isOrtho =      my::AlmostEqual(m2::DotProduct(segment, longer), .0f);
        if (isCodirected && !isOrtho)
          continue;
      }

      vec2 normal(-segment.y, segment.x);
      ASSERT(my::AlmostEqual(m2::DotProduct(normal, segment), 0.f), ());

      normal = normal.Normalize()*hw;
      ASSERT(!normal.IsAlmostZero(), (i, normal, start, end, segment));

      ToPoint3DFunctor convertTo3d(m_depth);

      m_points.push_back(convertTo3d(start));
      m_points.push_back(convertTo3d(start));
      m_normals.push_back(convertTo3d(normal));
      m_normals.push_back(convertTo3d(-normal));


      m_points.push_back(convertTo3d(end));
      m_points.push_back(convertTo3d(end));
      m_normals.push_back(convertTo3d(normal));
      m_normals.push_back(convertTo3d(-normal));

      start = end;
    }
  }

  void LineShape::Draw(RefPointer<Batcher> batcher) const
  {
    GLState state(gpu::SOLID_LINE_PROGRAM, 0, TextureBinding("", false, 0, MakeStackRefPointer<Texture>(NULL)));
    float r, g, b, a;
    ::Convert(m_color, r, g, b, a);
    state.GetUniformValues().SetFloatValue("color", r, g, b, a);

    AttributeProvider provider(2, m_points.size());

    {
      BindingInfo positionInfo(1);
      BindingDecl & decl = positionInfo.GetBindingDecl(0);
      decl.m_attributeName = "position";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;

      provider.InitStream(0, positionInfo, MakeStackRefPointer((void*)&m_points[0]));
    }

    {
      BindingInfo normalInfo(1);
      BindingDecl & decl = normalInfo.GetBindingDecl(0);
      decl.m_attributeName = "normal";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;

      provider.InitStream(1, normalInfo, MakeStackRefPointer((void*)&m_normals[0]));
    }

    batcher->InsertTriangleStrip(state, MakeStackRefPointer(&provider));
  }
}

