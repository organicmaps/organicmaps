#include "testing/testing.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/route_shape.hpp"

#include "drape/utils/projection.hpp"

#include <algorithm>
#include <vector>

namespace route_shape_tests
{
UNIT_TEST(RouteShape_BoundaryMarkerCoversJoiningLines)
{
  using Shape = df::RouteShape;
  std::vector<Shape::GeometryBufferData<Shape::GeometryBuffer>> incoming;
  Shape::PrepareGeometry({{-0.00003, -0.00001}, {-0.00002, 0}, {-0.00001, 0}, {0, 0}}, {}, {}, 200.0f, incoming);
  TEST(!incoming.empty(), ());
  float incomingDepth = 0.0f;
  for (auto const & data : incoming)
  {
    for (auto const & vertex : data.m_geometry)
      incomingDepth = std::max(incomingDepth, vertex.m_position.z);
    for (auto const & vertex : data.m_joinsGeometry)
      incomingDepth = std::max(incomingDepth, vertex.m_position.z);
  }

  df::SubrouteMarker marker;
  marker.m_colors = {df::GetTransitColorName("#ff0000"), df::GetTransitColorName("#0000ff")};
  marker.m_innerColor = df::GetTransitColorName("#ffffff");
  marker.m_scale = 4.0f;
  Shape::MarkersGeometryBuffer current;
  Shape::PrepareMarkersGeometry({marker}, {}, 100.0f, current);
  TEST_EQUAL(current.size(), 9, ());
  TEST_GREATER(current.front().m_position.z, 0.0f, ());
  TEST_GREATER(current.back().m_position.z, current.front().m_position.z, ());

  marker.m_baseDepthIndex = 0.0;
  Shape::MarkersGeometryBuffer zeroOwner;
  Shape::PrepareMarkersGeometry({marker}, {}, 300.0f, zeroOwner);
  TEST_EQUAL(zeroOwner.size(), current.size(), ());
  for (size_t i = 0; i < current.size(); ++i)
  {
    TEST(zeroOwner[i].m_position == current[i].m_position, ());
    TEST(zeroOwner[i].m_normal == current[i].m_normal, ());
    TEST(zeroOwner[i].m_color == current[i].m_color, ());
  }

  // The ring is submitted with the next range, but must cover its incoming line as well.
  marker.m_baseDepthIndex = 1.0;
  Shape::MarkersGeometryBuffer boundary;
  Shape::PrepareMarkersGeometry({marker}, {}, 100.0f, boundary);
  TEST_EQUAL(boundary.size(), current.size(), ());
  for (auto const api : {dp::ApiVersion::OpenGLES3, dp::ApiVersion::Metal})
  {
    auto const projection = dp::MakeProjection(api, 0, 1024, 1024, 0);
    auto const projectedDepth = [&](float depth) { return depth * projection[10] + projection[11]; };
    for (auto const & vertex : boundary)
      TEST_LESS(projectedDepth(vertex.m_position.z), projectedDepth(incomingDepth), ());
    // Depth order between outer colors and the neutral center remains intact.
    TEST_LESS(projectedDepth(boundary.back().m_position.z), projectedDepth(boundary.front().m_position.z), ());
  }
}
}  // namespace route_shape_tests
