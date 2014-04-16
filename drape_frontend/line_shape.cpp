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

LineShape::LineShape(vector<m2::PointF> const & points,
                     LineViewParams const & params)
  : m_points(points)
  , m_params(params)
{
  ASSERT_GREATER(m_points.size(), 1, ());
}

void LineShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> /*textures*/) const
{
  //join, cap, segment params
  // vertex type:
  // [x] - {-1 : cap, 0 : segment, +1 : join}
  // [y] - cap: { 0 : round, 1 : butt, -1 : square}
  // [z] - join : {0: none, round : 1, bevel : 2}
  // [w] - unused
  float const T_CAP = -1.f;
  float const T_SEGMENT = 0.f;
  float const T_JOIN = +1.f;

  float const MIN_JOIN_ANGLE = 15*math::pi/180;
  float const MAX_JOIN_ANGLE = 179*math::pi/180;

  typedef m2::PointF vec2;

  size_t const count = m_points.size();

  // prepare data
  vector<Point3D> renderPoints;
  renderPoints.reserve(count);
  vector<Point3D> renderDirections;
  renderDirections.reserve(count);
  vector<Point3D> renderVertexTypes;
  renderVertexTypes.reserve(count);
  //

  float const hw = GetWidth() / 2.0f;
  float const realWidth = GetWidth();

  // We understand 'butt' cap as 'flat'
  bool const doAddCap = m_params.m_cap != ButtCap;
  vec2       direction  = m_points[1] - m_points[0];
  m2::PointF firstPoint = m_points[0];
  // Add start cap quad
  if (doAddCap)
  {
    renderPoints.push_back(Point3D::From2D(firstPoint, m_params.m_depth)); // A
    renderDirections.push_back(Point3D::From2D(direction, hw));
    renderVertexTypes.push_back(Point3D(T_CAP, m_params.m_cap, realWidth));

    renderPoints.push_back(Point3D::From2D(firstPoint, m_params.m_depth)); // B
    renderDirections.push_back(Point3D::From2D(direction, -hw));
    renderVertexTypes.push_back(Point3D(T_CAP, m_params.m_cap, realWidth));
  }
  //

  m2::PointF start = m_points[0];
  for (size_t i = 1; i < count; ++i)
  {
    m2::PointF end = m_points[i];
    vec2 segment   = end - start;

    if (i < count - 1)
    {
      vec2 longer = m_points[i+1] - start;
      float const dp = m2::DotProduct(segment, longer);
      bool  const isCodirected = my::AlmostEqual(dp, static_cast<float>(
                                                    m2::PointF::LengthFromZero(segment)
                                                  * m2::PointF::LengthFromZero(longer)));

      // We must not skip and unite with zero-length vectors.
      // For instance, if we have points [A,B,A]
      // we could have 'segment' = AB, and 'longer' = AA
      // if we unite it, we will lose AB segment.
      // We could use isAlmostZero, but dotProduct is faster.
      bool const isOrtho = my::AlmostEqual(dp, .0f);
      if (isCodirected && !isOrtho)
        continue;
    }

    Point3D const start3d = Point3D::From2D(start, m_params.m_depth);
    Point3D const end3d   = Point3D::From2D(end, m_params.m_depth);

    Point3D const directionPos = Point3D::From2D(segment, hw);
    Point3D const directionNeg = Point3D::From2D(segment, -hw);

    renderPoints.push_back(start3d);
    renderDirections.push_back(directionPos);
    renderVertexTypes.push_back(Point3D(T_SEGMENT, 0, realWidth));

    renderPoints.push_back(start3d);
    renderDirections.push_back(directionNeg);
    renderVertexTypes.push_back(Point3D(T_SEGMENT, 0, realWidth));

    renderPoints.push_back(end3d);
    renderDirections.push_back(directionPos);
    renderVertexTypes.push_back(Point3D(T_SEGMENT, 0, realWidth));

    renderPoints.push_back(end3d);
    renderDirections.push_back(directionNeg);
    renderVertexTypes.push_back(Point3D(T_SEGMENT, 0, realWidth));

    // This is a join!
    bool const needJoinSegments = (end != m_points[count - 1]);
    if (needJoinSegments && (m_params.m_join != BevelJoin))
    {
      vec2 vIn  = m_points[i]   - m_points[i-1];
      vec2 vOut = m_points[i+1] - m_points[i];
      float const cross = vIn.x*vOut.y - vIn.y*vOut.x;
      float const dot   = vIn.x*vOut.x + vIn.y*vOut.y;
      float const joinAngle = atan2(cross, dot);
      bool  const clockWise = cross < 0;
      float const directionFix = ( clockWise ? +1 : -1 );

      float const absAngle = my::Abs(joinAngle);
      if (absAngle > MIN_JOIN_ANGLE && absAngle < MAX_JOIN_ANGLE)
      {
        float const joinHeight = (m_params.m_join == MiterJoin)
                                 ? my::Abs(hw / cos(joinAngle/2))
                                 : 2*hw; // ensure we have enough space for sector

        // Add join triangles
        Point3D pivot = Point3D::From2D(m_points[i], m_params.m_depth);

        // T123
        vec2 nIn(-vIn.y, vIn.x);
        vec2 nInFixed = nIn.Normalize() * directionFix;

        renderPoints.push_back(pivot);//1
        renderDirections.push_back(Point3D::From2D(nInFixed, hw));
        renderVertexTypes.push_back(Point3D(T_JOIN, m_params.m_join, realWidth));

        renderPoints.push_back(pivot);//2
        renderDirections.push_back(Point3D(0,0,0)); // zero-shift point
        renderVertexTypes.push_back(Point3D(T_JOIN, m_params.m_join, realWidth));

        // T234
        vec2 nOut(-vOut.y, vOut.x);
        vec2 nOutFixed = nOut.Normalize() * directionFix;

        vec2 joinBackBisec = (nInFixed + nOutFixed).Normalize();

        renderPoints.push_back(pivot); //3
        renderDirections.push_back(Point3D::From2D(joinBackBisec, joinHeight));
        renderVertexTypes.push_back(Point3D(T_JOIN, m_params.m_join, realWidth));

        renderPoints.push_back(pivot); //4
        renderDirections.push_back(Point3D::From2D(nOutFixed, hw));
        renderVertexTypes.push_back(Point3D(T_JOIN, m_params.m_join, realWidth));

        if (!clockWise)
        {
          // We use triangle strip, so we need to create zero-sqare triangle
          // for correct rasterization.
          renderPoints.push_back(pivot); //4 second time
          renderDirections.push_back(Point3D::From2D(nOutFixed, hw));
          renderVertexTypes.push_back(Point3D(T_JOIN, m_params.m_join, realWidth));
        }
      }
    }
    //

    start = end;
  }

  //Add final cap
  if (doAddCap)
  {
    vec2 lastSegment     = m_points[count-1] - m_points[count-2];
    m2::PointF lastPoint = m_points[count-1];
    direction = -lastSegment;

    renderPoints.push_back(Point3D::From2D(lastPoint, m_params.m_depth)); // A
    renderDirections.push_back(Point3D::From2D(direction, -hw));
    renderVertexTypes.push_back(Point3D(T_CAP, m_params.m_cap, realWidth));

    renderPoints.push_back(Point3D::From2D(lastPoint, m_params.m_depth)); // B
    renderDirections.push_back(Point3D::From2D(direction, hw));
    renderVertexTypes.push_back(Point3D(T_CAP, m_params.m_cap, realWidth));
  }
  //

  GLState state(gpu::SOLID_LINE_PROGRAM, GLState::GeometryLayer);
  state.SetColor(GetColor());

  AttributeProvider provider(3, renderPoints.size());

  {
    BindingInfo positionInfo(1);
    BindingDecl & decl = positionInfo.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(0, positionInfo, MakeStackRefPointer((void*)&renderPoints[0]));
  }

  {
    BindingInfo directionInfo(1);
    BindingDecl & decl = directionInfo.GetBindingDecl(0);
    decl.m_attributeName = "a_direction";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(1, directionInfo, MakeStackRefPointer((void*)&renderDirections[0]));
  }

  {
    BindingInfo vertexTypeInfo(1);
    BindingDecl & decl = vertexTypeInfo.GetBindingDecl(0);
    decl.m_attributeName = "a_vertType";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(2, vertexTypeInfo, MakeStackRefPointer((void*)&renderVertexTypes[0]));
  }

  batcher->InsertTriangleStrip(state, MakeStackRefPointer(&provider));
}

} // namespace df

