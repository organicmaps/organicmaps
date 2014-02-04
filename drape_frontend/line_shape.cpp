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
  namespace
  {
    struct MatchesPrevious
    {
      MatchesPrevious(m2::PointF const & first)
        : m_prev(first)
      {}

      bool operator ()(m2::PointF const & current)
      {
        if (m2::AlmostEqual(m_prev, current))
          return true;
        else
        {
          m_prev = current;
          return false;
        }
      }

    private:
      m2::PointF m_prev;
    };
  }


  LineShape::LineShape(const vector<m2::PointF> & points,
                       float depth,
                       const LineViewParams & params)
    : m_depth(depth)
    , m_params(params)
  {
    ASSERT(points.size() > 1, ());
    m_points = points;
  }

  void LineShape::Draw(RefPointer<Batcher> batcher) const
  {
    vector<Point3D> renderPoints;
    vector<Point3D> renderNormals;

    const float hw = GetWidth() / 2.0f;
    typedef m2::PointF vec2;

    vec2 start = m_points[0];
    for (size_t i = 1; i < m_points.size(); ++i)
    {
      vec2 end = m_points[i];
      vec2 segment = end - start;

      if (i < m_points.size() - 1)
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

      ToPoint3DFunctor convertTo3d(m_depth);
      const Point3D start3d = convertTo3d(start);
      const Point3D end3d = convertTo3d(end);
      convertTo3d.SetThirdComponent(hw);
      const Point3D normalPos = convertTo3d(segment);
      const Point3D normalNeg = convertTo3d(-segment);

      renderPoints.push_back(start3d);
      renderPoints.push_back(start3d);
      renderNormals.push_back(normalPos);
      renderNormals.push_back(normalNeg);

      renderPoints.push_back(end3d);
      renderPoints.push_back(end3d);
      renderNormals.push_back(normalPos);
      renderNormals.push_back(normalNeg);

      start = end;
    }


    GLState state(gpu::SOLID_LINE_PROGRAM, 0, TextureBinding("", false, 0, MakeStackRefPointer<Texture>(NULL)));
    float r, g, b, a;
    ::Convert(GetColor(), r, g, b, a);
    state.GetUniformValues().SetFloatValue("color", r, g, b, a);

    AttributeProvider provider(2, renderPoints.size());

    {
      BindingInfo positionInfo(1);
      BindingDecl & decl = positionInfo.GetBindingDecl(0);
      decl.m_attributeName = "position";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;

      provider.InitStream(0, positionInfo, MakeStackRefPointer((void*)&renderPoints[0]));
    }

    {
      BindingInfo normalInfo(1);
      BindingDecl & decl = normalInfo.GetBindingDecl(0);
      decl.m_attributeName = "direction";
      decl.m_componentCount = 3;
      decl.m_componentType = GLConst::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;

      provider.InitStream(1, normalInfo, MakeStackRefPointer((void*)&renderNormals[0]));
    }

    batcher->InsertTriangleStrip(state, MakeStackRefPointer(&provider));
  }
}

