#include "render/route_shape.hpp"

#include "base/logging.hpp"

namespace rg
{

namespace
{

float const kLeftSide = 1.0;
float const kCenter = 0.0;
float const kRightSide = -1.0;

uint32_t const kMaxIndices = 15000;

enum EPointType
{
  StartPoint = 0,
  EndPoint = 1,
  PointsCount = 2
};

enum ENormalType
{
  StartNormal = 0,
  EndNormal = 1,
  BaseNormal = 2
};

struct LineSegment
{
  m2::PointF m_points[PointsCount];
  m2::PointF m_tangent;
  m2::PointF m_leftBaseNormal;
  m2::PointF m_leftNormals[PointsCount];
  m2::PointF m_rightBaseNormal;
  m2::PointF m_rightNormals[PointsCount];
  m2::PointF m_leftWidthScalar[PointsCount];
  m2::PointF m_rightWidthScalar[PointsCount];
  bool m_hasLeftJoin[PointsCount];
  bool m_generateJoin;

  LineSegment()
  {
    m_leftWidthScalar[StartPoint] = m_leftWidthScalar[EndPoint] = m2::PointF(1.0f, 0.0f);
    m_rightWidthScalar[StartPoint] = m_rightWidthScalar[EndPoint] = m2::PointF(1.0f, 0.0f);
    m_hasLeftJoin[StartPoint] = m_hasLeftJoin[EndPoint] = true;
    m_generateJoin = true;
  }
};

class RouteDataHolder
{
public:
  explicit RouteDataHolder(RouteData & data)
    : m_data(data), m_currentBuffer(0), m_indexCounter(0)
  {
    m_data.m_joinsBounds.clear();
    m_data.m_geometry.clear();
    m_data.m_boundingBoxes.clear();

    m_data.m_geometry.resize(1);
    m_data.m_boundingBoxes.resize(1);
  }

  void Check()
  {
    if (m_indexCounter > kMaxIndices)
    {
      m_data.m_geometry.emplace_back(pair<TGeometryBuffer, TIndexBuffer>());
      m_data.m_boundingBoxes.emplace_back(m2::RectD());

      m_indexCounter = 0;
      m_currentBuffer++;
    }
  }

  uint16_t GetIndexCounter() const { return m_indexCounter; }
  void IncrementIndexCounter(uint16_t value) { m_indexCounter += value; }

  void AddVertex(TRV && vertex)
  {
    m2::PointF p = vertex.pt + vertex.normal * vertex.length.x;
    m_data.m_boundingBoxes[m_currentBuffer].Add(m2::PointD(p.x, p.y));

    m_data.m_geometry[m_currentBuffer].first.push_back(move(vertex));
  }

  void AddIndex(uint16_t index)
  {
    m_data.m_geometry[m_currentBuffer].second.push_back(index);
  }

  void AddJoinBounds(RouteJoinBounds && bounds)
  {
    m_data.m_joinsBounds.push_back(move(bounds));
  }

private:
  RouteData & m_data;
  uint32_t m_currentBuffer;
  uint16_t m_indexCounter;
  m2::RectD m_boundingBox;
};

class ArrowDataHolder
{
public:
  explicit ArrowDataHolder(ArrowsBuffer & data) : m_data(data) {}

  void Check(){}
  uint16_t GetIndexCounter() const { return m_data.m_indexCounter; }
  void IncrementIndexCounter(uint16_t value) { m_data.m_indexCounter += value; }

  void AddVertex(TRV && vertex)
  {
    m_data.m_geometry.push_back(move(vertex));
  }

  void AddIndex(uint16_t index)
  {
    m_data.m_indices.push_back(index);
  }

  void AddJoinBounds(RouteJoinBounds && bounds) {}

private:
  ArrowsBuffer & m_data;
};

void UpdateNormalBetweenSegments(LineSegment * segment1, LineSegment * segment2)
{
  ASSERT(segment1 != nullptr, ());
  ASSERT(segment2 != nullptr, ());

  float const dotProduct = m2::DotProduct(segment1->m_leftNormals[EndPoint],
                                          segment2->m_leftNormals[StartPoint]);
  float const absDotProduct = fabs(dotProduct);
  float const kEps = 1e-5;

  if (fabs(absDotProduct - 1.0f) < kEps)
  {
    // change nothing
    return;
  }

  float const kMaxScalar = 5;
  float const crossProduct = m2::CrossProduct(segment1->m_tangent, segment2->m_tangent);
  if (crossProduct < 0)
  {
    segment1->m_hasLeftJoin[EndPoint] = true;
    segment2->m_hasLeftJoin[StartPoint] = true;

    // change right-side normals
    m2::PointF averageNormal = (segment1->m_rightNormals[EndPoint] + segment2->m_rightNormals[StartPoint]).Normalize();
    float const cosAngle = m2::DotProduct(segment1->m_tangent, averageNormal);
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
    m2::PointF averageNormal = (segment1->m_leftNormals[EndPoint] + segment2->m_leftNormals[StartPoint]).Normalize();
    float const cosAngle = m2::DotProduct(segment1->m_tangent, averageNormal);
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

void CalculateTangentAndNormals(m2::PointF const & pt0, m2::PointF const & pt1,
                                m2::PointF & tangent, m2::PointF & leftNormal,
                                m2::PointF & rightNormal)
{
  tangent = (pt1 - pt0).Normalize();
  leftNormal = m2::PointF(-tangent.y, tangent.x);
  rightNormal = -leftNormal;
}

void ConstructLineSegments(vector<m2::PointD> const & path, vector<LineSegment> & segments)
{
  ASSERT_LESS(1, path.size(), ());

  float const eps = 1e-5;

  m2::PointD prevPoint = path[0];
  for (size_t i = 1; i < path.size(); ++i)
  {
    // filter the same points
    if (prevPoint.EqualDxDy(path[i], eps))
      continue;

    LineSegment segment;

    segment.m_points[StartPoint] = m2::PointF(prevPoint.x, prevPoint.y);
    segment.m_points[EndPoint] = m2::PointF(path[i].x, path[i].y);
    CalculateTangentAndNormals(segment.m_points[StartPoint], segment.m_points[EndPoint], segment.m_tangent,
                               segment.m_leftBaseNormal, segment.m_rightBaseNormal);

    segment.m_leftNormals[StartPoint] = segment.m_leftNormals[EndPoint] = segment.m_leftBaseNormal;
    segment.m_rightNormals[StartPoint] = segment.m_rightNormals[EndPoint] = segment.m_rightBaseNormal;

    prevPoint = path[i];

    segments.push_back(segment);
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

void GenerateJoinNormals(m2::PointF const & normal1, m2::PointF const & normal2,
                         bool isLeft, vector<m2::PointF> & normals)
{
  float const eps = 1e-5;
  float const dotProduct = m2::DotProduct(normal1, normal2);
  if (fabs(dotProduct - 1.0f) < eps)
    return;

  float const segmentAngle = math::pi / 8.0;
  float const fullAngle = acos(dotProduct);
  int segmentsCount = max(static_cast<int>(fullAngle / segmentAngle), 1);

  float const angle = fullAngle / segmentsCount * (isLeft ? -1.0 : 1.0);
  m2::PointF const startNormal = normal1.Normalize();

  for (int i = 0; i < segmentsCount; i++)
  {
    m2::PointF n1 = m2::Rotate(startNormal, i * angle);
    m2::PointF n2 = m2::Rotate(startNormal, (i + 1) * angle);

    normals.push_back(m2::PointF::Zero());
    normals.push_back(isLeft ? n1 : n2);
    normals.push_back(isLeft ? n2 : n1);
  }
}

void GenerateCapNormals(m2::PointF const & normal, bool isStart, vector<m2::PointF> & normals)
{
  int const segmentsCount = 8;
  float const segmentSize = static_cast<float>(math::pi) / segmentsCount * (isStart ? -1.0 : 1.0);
  m2::PointF const startNormal = normal.Normalize();

  for (int i = 0; i < segmentsCount; i++)
  {
    m2::PointF n1 = m2::Rotate(startNormal, i * segmentSize);
    m2::PointF n2 = m2::Rotate(startNormal, (i + 1) * segmentSize);

    normals.push_back(m2::PointF::Zero());
    normals.push_back(isStart ? n1 : n2);
    normals.push_back(isStart ? n2 : n1);
  }
}

m2::PointF GetNormal(LineSegment const & segment, bool isLeft, ENormalType normalType)
{
  if (normalType == BaseNormal)
    return isLeft ? segment.m_leftBaseNormal : segment.m_rightBaseNormal;

  int const index = (normalType == StartNormal) ? StartPoint : EndPoint;
  return isLeft ? segment.m_leftNormals[index] * segment.m_leftWidthScalar[index].x:
                  segment.m_rightNormals[index] * segment.m_rightWidthScalar[index].x;
}

template<typename TRouteDataHolder>
double GenerateGeometry(vector<m2::PointD> const & points, bool isRoute, double lengthScalar,
                        TRouteDataHolder & routeDataHolder)
{
  float depth = 0.0f;

  auto const generateTriangles = [&routeDataHolder, &depth](m2::PointF const & pivot, vector<m2::PointF> const & normals,
                                                            m2::PointF const & length, bool isLeft)
  {
    float const eps = 1e-5;
    size_t const trianglesCount = normals.size() / 3;
    float const side = isLeft ? kLeftSide : kRightSide;
    for (int j = 0; j < trianglesCount; j++)
    {
      float const lenZ1 = normals[3 * j].Length() < eps ? kCenter : side;
      float const lenZ2 = normals[3 * j + 1].Length() < eps ? kCenter : side;
      float const lenZ3 = normals[3 * j + 2].Length() < eps ? kCenter : side;

      routeDataHolder.Check();

      routeDataHolder.AddVertex(TRV(pivot, depth, normals[3 * j], length, lenZ1));
      routeDataHolder.AddVertex(TRV(pivot, depth, normals[3 * j + 1], length, lenZ2));
      routeDataHolder.AddVertex(TRV(pivot, depth, normals[3 * j + 2], length, lenZ3));

      uint16_t indexCounter = routeDataHolder.GetIndexCounter();
      routeDataHolder.AddIndex(indexCounter);
      routeDataHolder.AddIndex(indexCounter + 1);
      routeDataHolder.AddIndex(indexCounter + 2);
      routeDataHolder.IncrementIndexCounter(3);
    }
  };

  auto const generateIndices = [&routeDataHolder]()
  {
    uint16_t indexCounter = routeDataHolder.GetIndexCounter();
    routeDataHolder.AddIndex(indexCounter);
    routeDataHolder.AddIndex(indexCounter + 1);
    routeDataHolder.AddIndex(indexCounter + 3);
    routeDataHolder.AddIndex(indexCounter + 3);
    routeDataHolder.AddIndex(indexCounter + 2);
    routeDataHolder.AddIndex(indexCounter);
    routeDataHolder.IncrementIndexCounter(4);
  };

  // constuct segments
  vector<LineSegment> segments;
  segments.reserve(points.size() - 1);
  ConstructLineSegments(points, segments);

  // build geometry
  float length = 0;
  vector<m2::PointF> normals;
  normals.reserve(24);
  for (size_t i = 0; i < segments.size(); i++)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < segments.size() - 1) ? &segments[i + 1] : nullptr);

    // generate main geometry
    m2::PointF const startPivot = segments[i].m_points[StartPoint];
    m2::PointF const endPivot = segments[i].m_points[EndPoint];

    float const endLength = length + (endPivot - startPivot).Length();

    m2::PointF const leftNormalStart = GetNormal(segments[i], true /* isLeft */, StartNormal);
    m2::PointF const rightNormalStart = GetNormal(segments[i], false /* isLeft */, StartNormal);
    m2::PointF const leftNormalEnd = GetNormal(segments[i], true /* isLeft */, EndNormal);
    m2::PointF const rightNormalEnd = GetNormal(segments[i], false /* isLeft */, EndNormal);

    float projLeftStart = 0.0;
    float projLeftEnd = 0.0;
    float projRightStart = 0.0;
    float projRightEnd = 0.0;
    float scaledLength = length / lengthScalar;
    float scaledEndLength = endLength / lengthScalar;
    if (isRoute)
    {
      projLeftStart = -segments[i].m_leftWidthScalar[StartPoint].y / lengthScalar;
      projLeftEnd = segments[i].m_leftWidthScalar[EndPoint].y / lengthScalar;
      projRightStart = -segments[i].m_rightWidthScalar[StartPoint].y / lengthScalar;
      projRightEnd = segments[i].m_rightWidthScalar[EndPoint].y / lengthScalar;
    }
    else
    {
      float const arrowTailEndCoord = arrowTailSize;
      float const arrowHeadStartCoord = 1.0 - arrowHeadSize;
      if (i == 0)
      {
        scaledLength = 0.0f;
        scaledEndLength = arrowTailEndCoord;
      }
      else if (i == segments.size() - 1)
      {
        scaledLength = arrowHeadStartCoord;
        scaledEndLength = 1.0f;
      }
      else
      {
        scaledLength = arrowTailEndCoord + arrowTailSize;
        scaledEndLength = arrowTailEndCoord + arrowTailSize * 2;
      }
    }

    routeDataHolder.Check();

    routeDataHolder.AddVertex(TRV(startPivot, depth, m2::PointF::Zero(), m2::PointF(scaledLength, 0), kCenter));
    routeDataHolder.AddVertex(TRV(startPivot, depth, leftNormalStart, m2::PointF(scaledLength, projLeftStart), kLeftSide));
    routeDataHolder.AddVertex(TRV(endPivot, depth, m2::PointF::Zero(), m2::PointF(scaledEndLength, 0), kCenter));
    routeDataHolder.AddVertex(TRV(endPivot, depth, leftNormalEnd, m2::PointF(scaledEndLength, projLeftEnd), kLeftSide));
    generateIndices();

    routeDataHolder.AddVertex(TRV(startPivot, depth, rightNormalStart, m2::PointF(scaledLength, projRightStart), kRightSide));
    routeDataHolder.AddVertex(TRV(startPivot, depth, m2::PointF::Zero(), m2::PointF(scaledLength, 0), kCenter));
    routeDataHolder.AddVertex(TRV(endPivot, depth, rightNormalEnd, m2::PointF(scaledEndLength, projRightEnd), kRightSide));
    routeDataHolder.AddVertex(TRV(endPivot, depth, m2::PointF::Zero(), m2::PointF(scaledEndLength, 0), kCenter));
    generateIndices();

    // generate joins
    if (segments[i].m_generateJoin && i < segments.size() - 1)
    {
      normals.clear();
      m2::PointF n1 = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_leftNormals[EndPoint] :
                                                            segments[i].m_rightNormals[EndPoint];
      m2::PointF n2 = segments[i + 1].m_hasLeftJoin[StartPoint] ? segments[i + 1].m_leftNormals[StartPoint] :
                                                                  segments[i + 1].m_rightNormals[StartPoint];
      GenerateJoinNormals(n1, n2, segments[i].m_hasLeftJoin[EndPoint], normals);
      generateTriangles(endPivot, normals, m2::PointF(scaledEndLength, 0), segments[i].m_hasLeftJoin[EndPoint]);
    }

    // generate caps
    if (isRoute && i == 0)
    {
      normals.clear();
      GenerateCapNormals(segments[i].m_rightNormals[StartPoint], true /* isStart */, normals);
      generateTriangles(startPivot, normals, m2::PointF(scaledLength, 0), true);
    }

    if (isRoute && i == segments.size() - 1)
    {
      normals.clear();
      GenerateCapNormals(segments[i].m_rightNormals[EndPoint], false /* isStart */, normals);
      generateTriangles(endPivot, normals, m2::PointF(scaledEndLength, 0), true);
    }

    length = endLength;
  }

  // calculate joins bounds
  if (isRoute)
  {
    float const eps = 1e-5;
    double len = 0;
    for (size_t i = 0; i + 1 < segments.size(); i++)
    {
      len += (segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]).Length();

      RouteJoinBounds bounds;
      bounds.m_start = min(segments[i].m_leftWidthScalar[EndPoint].y,
                           segments[i].m_rightWidthScalar[EndPoint].y);
      bounds.m_end = max(-segments[i + 1].m_leftWidthScalar[StartPoint].y,
                         -segments[i + 1].m_rightWidthScalar[StartPoint].y);

      if (fabs(bounds.m_end - bounds.m_start) < eps)
        continue;

      bounds.m_offset = len;
      routeDataHolder.AddJoinBounds(move(bounds));
    }
  }

  return length;
}

}

void RouteShape::PrepareGeometry(m2::PolylineD const & polyline, RouteData & output)
{
  vector<m2::PointD> const & path = polyline.GetPoints();
  ASSERT_LESS(1, path.size(), ());

  RouteDataHolder holder(output);
  output.m_length = GenerateGeometry(path, true /* isRoute */, 1.0 /* lengthScalar */, holder);
  ASSERT_EQUAL(output.m_geometry.size(), output.m_boundingBoxes.size(), ());
}

void RouteShape::PrepareArrowGeometry(vector<m2::PointD> const & points,
                                      double start, double end, ArrowsBuffer & output)
{
  ArrowDataHolder holder(output);
  GenerateGeometry(points, false /* isRoute */, end - start, holder);
}

} // namespace rg

