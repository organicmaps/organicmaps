#include "testing/testing.hpp"

#include "topography_generator/marching_squares/square.hpp"

#include "geometry/mercator.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace
{
class CornerValues : public topography_generator::ValuesProvider<int16_t>
{
public:
  explicit CornerValues(std::array<int16_t, 4> const & values) : m_values(values) {}

  int16_t GetValue(ms::LatLon const & p) override
  {
    if (p.m_lon == 10.0)
      return m_values[p.m_lat == 2.0 ? 0 : 1];
    return m_values[p.m_lat == 6.0 ? 2 : 3];
  }

  int16_t GetInvalidValue() const override { return std::numeric_limits<int16_t>::min(); }

private:
  std::array<int16_t, 4> m_values;
};

UNIT_TEST(MarchingSquares_InterpolateEdges)
{
  struct TestCase
  {
    std::array<int16_t, 4> m_values;  // Left-bottom, left-top, right-top, right-bottom.
    std::array<ms::LatLon, 2> m_expected;
  };
  TestCase const cases[] = {
      {{9, 1, 1, 1}, {ms::LatLon(4, 10), ms::LatLon(2, 13)}},  // Left -> bottom.
      {{1, 9, 1, 1}, {ms::LatLon(6, 13), ms::LatLon(4, 10)}},  // Top -> left.
      {{1, 1, 9, 1}, {ms::LatLon(4, 16), ms::LatLon(6, 13)}},  // Right -> top.
      {{1, 1, 1, 9}, {ms::LatLon(2, 13), ms::LatLon(4, 16)}}   // Bottom -> right.
  };

  for (auto const & test : cases)
  {
    CornerValues values(test.m_values);
    topography_generator::Square<int16_t> square({2, 10}, {6, 16}, 5, 5, values, "interpolation");
    topography_generator::ContoursBuilder builder(1, "interpolation");
    builder.BeginLine();
    square.GenerateSegments(builder);
    builder.EndLine(true);

    std::unordered_map<int16_t, std::vector<topography_generator::Contour>> contours;
    builder.GetContours<int16_t>(5, 5, contours);
    TEST_EQUAL(contours.size(), 1, (test.m_values));
    auto const it = contours.find(5);
    TEST(it != contours.end(), ());
    TEST_EQUAL(it->second.size(), 1, ());
    auto const & contour = it->second.front();
    TEST_EQUAL(contour.size(), test.m_expected.size(), ());
    for (size_t i = 0; i < contour.size(); ++i)
    {
      auto const actual = mercator::ToLatLon(contour[i]);
      TEST_ALMOST_EQUAL_ABS(actual.m_lat, test.m_expected[i].m_lat, 1e-9, (test.m_values, i));
      TEST_ALMOST_EQUAL_ABS(actual.m_lon, test.m_expected[i].m_lon, 1e-9, (test.m_values, i));
    }
  }
}
}  // namespace
