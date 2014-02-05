#include "line_shape.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"

namespace df
{
  LineShape::LineShape(const vector<m2::PointF> & points,
                       float depth,
                       const LineViewParams & params)
    : m_points(points)
    , m_depth(depth)
    , m_params(params)
  {
    ASSERT_GREATER(m_points.size(), 1, ());
  }

  void LineShape::Draw(RefPointer<Batcher> batcher) const
  {
    //join, cap, segment params
    // vertex type:
    // [x] - {-1 : cap, 0 : segment, +1 : join}
    // [y] - cap: { 0 : round, 1 : butt, -1 : square}
    // [z] - join : {0: none, round : 1, bevel : 2}
    // [w] - unused
    const float T_CAP = -1.f;
    const float T_SEGMENT = 0.f;
    const float T_JOIN = +1.f;

    typedef m2::PointF vec2;

    const size_t count = m_points.size();

    // prepare data
    vector<Point3D> renderPoints;
    renderPoints.reserve(count);
    vector<Point3D> renderDirections;
    renderDirections.reserve(count);
    vector<vec2> renderVertexTypes;
    renderVertexTypes.reserve(count);
    //

    const float hw = GetWidth() / 2.0f;

    // Add start cap quad
    vec2       direction  = m_points[1] - m_points[0];
    m2::PointF firstPoint = m_points[0];

    renderPoints.push_back(Point3D::From2D(firstPoint, m_depth)); // A
    renderDirections.push_back(Point3D::From2D(direction, hw));
    renderVertexTypes.push_back(vec2(T_CAP, m_params.m_cap));

    renderPoints.push_back(Point3D::From2D(firstPoint, m_depth)); // B
    renderDirections.push_back(Point3D::From2D(direction, -hw));
    renderVertexTypes.push_back(vec2(T_CAP, m_params.m_cap));
    //

    m2::PointF start = m_points[0];
    for (size_t i = 1; i < count; ++i)
    {
      m2::PointF end = m_points[i];
      vec2 segment   = end - start;

      if (i < count - 1)
      {
        vec2 longer = m_points[i+1] - start;
        const float dp = m2::DotProduct(segment, longer);
        const bool isCodirected = my::AlmostEqual(dp, static_cast<float>(
                                                      m2::PointF::LengthFromZero(segment)
                                                    * m2::PointF::LengthFromZero(longer)));

        // We must not skip and unite with zero-length vectors.
        // For instance, if we have points [A,B,A]
        // we could have 'segment' = AB, and 'longer' = AA
        // if we unite it, we will lose AB segment.
        // We could use isAlmostZero, but dotProduct is faster.
        const bool isOrtho = my::AlmostEqual(dp, .0f);
        if (isCodirected && !isOrtho)
          continue;
      }

      const Point3D start3d = Point3D::From2D(start, m_depth);
      const Point3D end3d   = Point3D::From2D(end, m_depth);

      const Point3D directionPos = Point3D::From2D(segment, hw);
      const Point3D directionNeg = Point3D::From2D(segment, -hw);

      renderPoints.push_back(start3d);
      renderDirections.push_back(directionPos);
      renderVertexTypes.push_back(vec2(T_SEGMENT, 0));

      renderPoints.push_back(start3d);
      renderDirections.push_back(directionNeg);
      renderVertexTypes.push_back(vec2(T_SEGMENT, 0));

      renderPoints.push_back(end3d);
      renderDirections.push_back(directionPos);
      renderVertexTypes.push_back(vec2(T_SEGMENT, 0));

      renderPoints.push_back(end3d);
      renderDirections.push_back(directionNeg);
      renderVertexTypes.push_back(vec2(T_SEGMENT, 0));

      start = end;
    }

    //Add final cap
    vec2 lastSegment     = m_points[count-1] - m_points[count-2];
    m2::PointF lastPoint = m_points[count-1];
    direction = -lastSegment;

    renderPoints.push_back(Point3D::From2D(lastPoint, m_depth)); // A
    renderDirections.push_back(Point3D::From2D(direction, -hw));
    renderVertexTypes.push_back(vec2(T_CAP, m_params.m_cap));

    renderPoints.push_back(Point3D::From2D(lastPoint, m_depth)); // B
    renderDirections.push_back(Point3D::From2D(direction, hw));
    renderVertexTypes.push_back(vec2(T_CAP, m_params.m_cap));
    //

    GLState state(gpu::SOLID_LINE_PROGRAM, 0, TextureBinding("", false, 0, MakeStackRefPointer<Texture>(NULL)));
    float r, g, b, a;
    ::Convert(GetColor(), r, g, b, a);
    state.GetUniformValues().SetFloatValue("u_color", r, g, b, a);

    AttributeProvider provider(3, renderPoints.size());

    {
      BindingInfo positionInfo(1);
      BindingDecl & decl = positionInfo.GetBindingDecl(0);
      decl.m_attributeName = "a_position";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;

      provider.InitStream(0, positionInfo, MakeStackRefPointer((void*)&renderPoints[0]));
    }

    {
      BindingInfo directionInfo(1);
      BindingDecl & decl = directionInfo.GetBindingDecl(0);
      decl.m_attributeName = "a_direction";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;

      provider.InitStream(1, directionInfo, MakeStackRefPointer((void*)&renderDirections[0]));
    }

    {
      BindingInfo vertexTypeInfo(1);
      BindingDecl & decl = vertexTypeInfo.GetBindingDecl(0);
      decl.m_attributeName = "a_vertType";
      decl.m_componentCount = 2;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;

      provider.InitStream(2, vertexTypeInfo, MakeStackRefPointer((void*)&renderVertexTypes[0]));
    }

    batcher->InsertTriangleStrip(state, MakeStackRefPointer(&provider));
  }
}

