#include "drape_frontend/line_shape_helper.hpp"

#include "drape/glsl_func.hpp"

#include "base/assert.hpp"
#include "geometry/parametrized_segment.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace df
{
glsl::vec2 InterpolateLinePixelOffset(std::vector<m2::PointD> const & path, std::vector<m2::PointF> const & offsets,
                                      size_t segment, m2::PointD const & point)
{
  ASSERT_EQUAL(path.size(), offsets.size(), ());
  segment = std::min(segment, path.size() - 2);
  double const length = (path[segment + 1] - path[segment]).Length();
  float const t = length == 0 ? 0 : std::clamp((point - path[segment]).Length() / length, 0.0, 1.0);
  return glsl::ToVec2(offsets[segment]) * (1 - t) + glsl::ToVec2(offsets[segment + 1]) * t;
}

float JunctionTaper::Scale(double distance) const
{
  double factor = 1;
  if (m_span > 0)
    for (double junction : m_distances)
      factor = std::min(factor, std::abs(distance - junction) / m_span);
  return factor * factor * (3 - 2 * factor);
}

JunctionTaper MakeJunctionTaper(m2::SharedSpline const & source, std::vector<m2::PointD> const & junctions, double span,
                                double tolerance)
{
  JunctionTaper result{source, {}, span};
  auto const & path = source->GetPath();
  if (junctions.empty() || path.size() < 2 || span <= 0 || tolerance <= 0)
    return result;
  std::vector<double> distances{0};
  for (size_t i = 1; i < path.size(); ++i)
    distances.push_back(distances.back() + (path[i] - path[i - 1]).Length());
  for (auto const & junction : junctions)
  {
    double best = tolerance * tolerance;
    double earliest = distances.back();
    double latest = 0;
    std::optional<double> position;
    for (size_t i = 1; i < path.size(); ++i)
    {
      auto const p = m2::ParametrizedSegment<m2::PointD>(path[i - 1], path[i]).ClosestPointTo(junction);
      double const error = (p - junction).SquaredLength();
      double const along = distances[i - 1] + (p - path[i - 1]).Length();
      if (error <= tolerance * tolerance)
      {
        earliest = std::min(earliest, along);
        latest = std::max(latest, along);
      }
      if (error <= best)
      {
        best = error;
        position = along;
      }
    }
    // The debug payload has coordinates, not a vertex identity in simplified
    // geometry. At loops/self-crossings an ambiguous match must leave a gap.
    if (position && latest - earliest <= 4 * tolerance)
      result.m_distances.push_back(*position);
  }
  if (result.m_distances.empty())
    return result;
  auto samples = distances;
  for (double junction : result.m_distances)
    for (int i = -4; i <= 4; ++i)
      samples.push_back(std::clamp(junction + span * i / 4, 0.0, distances.back()));
  std::sort(samples.begin(), samples.end());
  samples.erase(std::unique(samples.begin(), samples.end()), samples.end());
  std::vector<m2::PointD> points;
  size_t segment = 1;
  for (double distance : samples)
  {
    while (segment + 1 < distances.size() && distances[segment] < distance)
      ++segment;
    double const length = distances[segment] - distances[segment - 1];
    if (length <= 0)
      continue;
    double const t = (distance - distances[segment - 1]) / length;
    points.push_back(path[segment - 1] * (1 - t) + path[segment] * t);
  }
  if (points.size() >= 2)
    result.m_spline = m2::SharedSpline(std::move(points));
  return result;
}

double PathSymbolOffsetForClip(double first, double step, double sourceDistance)
{
  ASSERT_GREATER(step, 0.0, ());
  sourceDistance = std::max(0.0, sourceDistance);
  if (sourceDistance <= first)
    return first - sourceDistance;
  double result = std::fmod(first - sourceDistance, step);
  return result < 0 ? result + step : result;
}

glsl::vec2 LinePixelOffsetOnSegment(std::vector<m2::PointD> const & path, size_t segment, m2::PointD const & point,
                                    float offset)
{
  if (point.EqualDxDy(path[segment], 1e-12))
    return LinePixelOffset(path, segment, offset);
  if (point.EqualDxDy(path[segment + 1], 1e-12))
    return LinePixelOffset(path, segment + 1, offset);
  auto const delta = path[segment + 1] - path[segment];
  double const length = delta.Length();
  if (length == 0)
    return {0.0f, 0.0f};
  return glsl::vec2(delta.y * offset / length, -delta.x * offset / length);
}

float SideOffsetPixels(float roadWidth, float lineWidth, float gap, int side)
{
  return side * (0.5f * (roadWidth + lineWidth) + gap);
}

glsl::vec2 LinePixelOffset(std::vector<m2::PointD> const & path, size_t index, float offset)
{
  if (offset == 0)
    return {0.0f, 0.0f};
  ASSERT_GREATER(path.size(), 1, ());
  auto incoming = index == 0 ? path[1] - path[0] : path[index] - path[index - 1];
  auto outgoing = index + 1 == path.size() ? incoming : path[index + 1] - path[index];
  if (incoming.SquaredLength() == 0)
    incoming = outgoing;
  if (outgoing.SquaredLength() == 0)
    outgoing = incoming;
  if (incoming.SquaredLength() == 0)
    return {0.0f, 0.0f};
  incoming = incoming / incoming.Length();
  outgoing = outgoing / outgoing.Length();
  m2::PointD const a(incoming.y, -incoming.x), b(outgoing.y, -outgoing.x);
  double const denominator = 1.0 + DotProduct(incoming, outgoing);
  auto miter = denominator < 0.01 ? b : (a + b) / denominator;
  // Bound near-reversals rather than producing arbitrarily long spikes.
  double const length = miter.Length();
  if (length > 4.0)
    miter = miter * (4.0 / length);
  return glsl::vec2(miter.x * offset, miter.y * offset);
}

namespace
{
void UpdateNormalBetweenSegments(LineSegment * segment1, LineSegment * segment2)
{
  ASSERT(segment1 != nullptr && segment2 != nullptr, ());

  float const dotProduct = glsl::dot(segment1->m_leftNormals[EndPoint], segment2->m_leftNormals[StartPoint]);
  float const absDotProduct = fabs(dotProduct);
  float const kEps = 1e-5;

  if (fabs(absDotProduct - 1.0f) < kEps)
  {
    // change nothing
    return;
  }

  float const kMaxScalar = 5;
  float const crossProduct = glsl::cross(glsl::vec3(segment1->m_tangent, 0), glsl::vec3(segment2->m_tangent, 0)).z;
  if (crossProduct < 0)
  {
    segment1->m_hasLeftJoin[EndPoint] = true;
    segment2->m_hasLeftJoin[StartPoint] = true;

    // change right-side normals
    glsl::vec2 averageNormal =
        glsl::normalize(segment1->m_rightNormals[EndPoint] + segment2->m_rightNormals[StartPoint]);
    float const cosAngle = glsl::dot(segment1->m_tangent, averageNormal);
    float const widthScalar = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
    if (widthScalar < kMaxScalar)
    {
      segment1->m_rightNormals[EndPoint] = averageNormal;
      segment2->m_rightNormals[StartPoint] = averageNormal;
      segment1->m_rightWidthScalar[EndPoint].x = widthScalar;
      segment1->m_rightWidthScalar[EndPoint].y = widthScalar * cosAngle;
      segment2->m_rightWidthScalar[StartPoint] = segment1->m_rightWidthScalar[EndPoint];
    }
    else
    {
      segment1->m_generateJoin = false;
    }
  }
  else
  {
    segment1->m_hasLeftJoin[EndPoint] = false;
    segment2->m_hasLeftJoin[StartPoint] = false;

    // change left-side normals
    glsl::vec2 averageNormal = glsl::normalize(segment1->m_leftNormals[EndPoint] + segment2->m_leftNormals[StartPoint]);
    float const cosAngle = glsl::dot(segment1->m_tangent, averageNormal);
    float const widthScalar = 1.0f / sqrt(1.0f - cosAngle * cosAngle);
    if (widthScalar < kMaxScalar)
    {
      segment1->m_leftNormals[EndPoint] = averageNormal;
      segment2->m_leftNormals[StartPoint] = averageNormal;
      segment1->m_leftWidthScalar[EndPoint].x = widthScalar;
      segment1->m_leftWidthScalar[EndPoint].y = widthScalar * cosAngle;
      segment2->m_leftWidthScalar[StartPoint] = segment1->m_leftWidthScalar[EndPoint];
    }
    else
    {
      segment1->m_generateJoin = false;
    }
  }
}
}  // namespace

void CalculateTangentAndNormals(glsl::vec2 const & pt0, glsl::vec2 const & pt1, glsl::vec2 & tangent,
                                glsl::vec2 & leftNormal, glsl::vec2 & rightNormal)
{
  tangent = glsl::normalize(pt1 - pt0);
  leftNormal = glsl::vec2(-tangent.y, tangent.x);
  rightNormal = -leftNormal;
}

void ConstructLineSegments(std::vector<m2::PointD> const & path, std::vector<glsl::vec4> const & segmentsColors,
                           std::vector<LineSegment> & segments)
{
  ASSERT_LESS(1, path.size(), ());

  if (!segmentsColors.empty())
    ASSERT_EQUAL(segmentsColors.size() + 1, path.size(), ());

  m2::PointD prevPoint = path[0];
  for (size_t i = 1; i < path.size(); ++i)
  {
    m2::PointF const p1 = m2::PointF(prevPoint.x, prevPoint.y);
    m2::PointF const p2 = m2::PointF(path[i].x, path[i].y);
    if (p1.EqualDxDy(p2, 1.0E-5))
      continue;

    // Important! Do emplace_back first and fill parameters later.
    // Fill parameters first and push_back later will cause ugly bug in clang 3.6 -O3 optimization.
    segments.emplace_back(glsl::ToVec2(p1), glsl::ToVec2(p2));
    LineSegment & segment = segments.back();

    CalculateTangentAndNormals(glsl::ToVec2(p1), glsl::ToVec2(p2), segment.m_tangent, segment.m_leftBaseNormal,
                               segment.m_rightBaseNormal);

    segment.m_leftNormals[StartPoint] = segment.m_leftNormals[EndPoint] = segment.m_leftBaseNormal;
    segment.m_rightNormals[StartPoint] = segment.m_rightNormals[EndPoint] = segment.m_rightBaseNormal;
    if (!segmentsColors.empty())
      segment.m_color = segmentsColors[i - 1];

    prevPoint = path[i];
  }
}

void UpdateNormals(LineSegment * segment, LineSegment * prevSegment, LineSegment * nextSegment)
{
  ASSERT(segment != nullptr, ());

  if (prevSegment != nullptr)
    UpdateNormalBetweenSegments(prevSegment, segment);

  if (nextSegment != nullptr)
    UpdateNormalBetweenSegments(segment, nextSegment);
}

std::vector<glsl::vec2> GenerateJoinNormals(dp::LineJoin joinType, glsl::vec2 const & normal1,
                                            glsl::vec2 const & normal2, float halfWidth, bool isLeft, float widthScalar,
                                            std::vector<glsl::vec2> * uv)
{
  std::vector<glsl::vec2> normals;
  float const eps = 1e-5;
  if (fabs(glsl::dot(normal1, normal2) - 1.0f) < eps)
    return normals;

  if (joinType == dp::LineJoin::BevelJoin)
  {
    glsl::vec2 const n1 = halfWidth * normal1;
    glsl::vec2 const n2 = halfWidth * normal2;

    normals = {glsl::vec2(0.0f, 0.0f), isLeft ? n1 : n2, isLeft ? n2 : n1};

    if (uv)
    {
      *uv = {glsl::vec2(0.5f, 0.5f), isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f),
             isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f)};
    }
  }
  else if (joinType == dp::LineJoin::MiterJoin)
  {
    glsl::vec2 averageNormal = halfWidth * widthScalar * glsl::normalize(normal1 + normal2);

    glsl::vec2 const n1 = halfWidth * normal1;
    glsl::vec2 const n2 = halfWidth * normal2;

    normals = {glsl::vec2(0.0f, 0.0f), isLeft ? n1 : averageNormal, isLeft ? averageNormal : n1,
               glsl::vec2(0.0f, 0.0f), isLeft ? averageNormal : n2, isLeft ? n2 : averageNormal};

    if (uv)
    {
      *uv = {glsl::vec2(0.5f, 0.5f),
             isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f),
             isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f),

             glsl::vec2(0.5f, 0.5f),
             isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f),
             isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f)};
    }
  }
  else
  {
    double const segmentAngle = math::pi / 8.0;
    double const fullAngle = acos(glsl::dot(normal1, normal2));
    int segmentsCount = static_cast<int>(fullAngle / segmentAngle);
    if (segmentsCount == 0)
      segmentsCount = 1;

    double const angle = fullAngle / segmentsCount * (isLeft ? -1.0 : 1.0);
    glsl::vec2 const normalizedNormal = glsl::normalize(normal1);
    m2::PointD const startNormal(normalizedNormal.x, normalizedNormal.y);

    normals.reserve(segmentsCount * 3);
    if (uv)
      uv->reserve(segmentsCount * 3);

    for (int i = 0; i < segmentsCount; i++)
    {
      m2::PointD n1 = m2::Rotate(startNormal, i * angle) * halfWidth;
      m2::PointD n2 = m2::Rotate(startNormal, (i + 1) * angle) * halfWidth;

      normals.push_back(glsl::vec2(0.0f, 0.0f));
      normals.push_back(isLeft ? glsl::vec2(n1.x, n1.y) : glsl::vec2(n2.x, n2.y));
      normals.push_back(isLeft ? glsl::vec2(n2.x, n2.y) : glsl::vec2(n1.x, n1.y));

      if (uv)
      {
        uv->push_back(glsl::vec2(0.5f, 0.5f));
        uv->push_back(isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f));
        uv->push_back(isLeft ? glsl::vec2(0.5f, 0.0f) : glsl::vec2(0.5f, 1.0f));
      }
    }
  }

  return normals;
}

std::vector<glsl::vec2> GenerateCapNormals(dp::LineCap capType, glsl::vec2 const & normal1, glsl::vec2 const & normal2,
                                           glsl::vec2 const & direction, float halfWidth, bool isStart,
                                           int segmentsCount)
{
  std::vector<glsl::vec2> normals;
  if (capType == dp::ButtCap)
    return normals;

  if (capType == dp::SquareCap)
  {
    glsl::vec2 const n1 = halfWidth * normal1;
    glsl::vec2 const n2 = halfWidth * normal2;
    glsl::vec2 const n3 = halfWidth * (normal1 + direction);
    glsl::vec2 const n4 = halfWidth * (normal2 + direction);

    normals = {n2, isStart ? n4 : n1, isStart ? n1 : n4, n1, isStart ? n4 : n3, isStart ? n3 : n4};
  }
  else
  {
    double const segmentSize = math::pi / segmentsCount * (isStart ? -1.0 : 1.0);
    glsl::vec2 const normalizedNormal = glsl::normalize(normal2);
    m2::PointD const startNormal(normalizedNormal.x, normalizedNormal.y);

    normals.reserve(segmentsCount * 3);
    for (int i = 0; i < segmentsCount; i++)
    {
      m2::PointD n1 = m2::Rotate(startNormal, i * segmentSize) * halfWidth;
      m2::PointD n2 = m2::Rotate(startNormal, (i + 1) * segmentSize) * halfWidth;

      normals.push_back(glsl::vec2(0.0f, 0.0f));
      normals.push_back(isStart ? glsl::vec2(n1.x, n1.y) : glsl::vec2(n2.x, n2.y));
      normals.push_back(isStart ? glsl::vec2(n2.x, n2.y) : glsl::vec2(n1.x, n1.y));
    }
  }

  return normals;
}

glsl::vec2 GetNormal(LineSegment const & segment, bool isLeft, ENormalType normalType)
{
  if (normalType == BaseNormal)
    return isLeft ? segment.m_leftBaseNormal : segment.m_rightBaseNormal;

  int const index = (normalType == StartNormal) ? StartPoint : EndPoint;
  return isLeft ? segment.m_leftWidthScalar[index].x * segment.m_leftNormals[index]
                : segment.m_rightWidthScalar[index].x * segment.m_rightNormals[index];
}

float GetProjectionLength(glsl::vec2 const & newPoint, glsl::vec2 const & startPoint, glsl::vec2 const & endPoint)
{
  glsl::vec2 const v1 = endPoint - startPoint;
  glsl::vec2 const v2 = newPoint - startPoint;
  float const squareLen = glsl::dot(v1, v1);
  float const proj = glsl::dot(v1, v2) / squareLen;
  return sqrt(squareLen) * math::Clamp(proj, 0.0f, 1.0f);
}

double WrapDashPhase(double phase, double maskLength)
{
  ASSERT_GREATER(maskLength, 0.0, ());
  double const wrapped = std::fmod(phase, maskLength);
  return wrapped < 0.0 ? wrapped + maskLength : wrapped;
}

double AdvanceDashPhase(double phase, double distance, bool reversed)
{
  return phase + (reversed ? -distance : distance);
}
}  // namespace df
