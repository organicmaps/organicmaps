#include "testing/testing.hpp"

#include "drape_frontend/user_mark_shapes.hpp"

#include <vector>

UNIT_TEST(UserLineRenderParams_SkipsUnrenderableLines)
{
  for (auto const & points :
       std::vector<std::vector<m2::PointD>>{{}, {{0, 0}}, {{0, 0}, {0, 0}}, {{0, 0}, {1e-8, 0}, {0, 0}}})
  {
    df::UserLineRenderParams params;
    params.AddSpline(points);
    TEST(params.m_splines.empty(), (points));
  }
}

UNIT_TEST(UserLineRenderParams_NormalizesConsecutivePositions)
{
  df::UserLineRenderParams params;
  params.AddSpline({{0, 0}, {0, 0}, {1e-8, 0}, {1, 0}, {1, 0}, {1, 1}, {1, 1}});
  TEST_EQUAL(params.m_splines.size(), 1, ());
  TEST_EQUAL(params.m_splines.front()->GetPath(), (std::vector<m2::PointD>{{0, 0}, {1, 0}, {1, 1}}), ());
  TEST_EQUAL(params.m_splines.front()->GetLength(), 2.0, ());
}

UNIT_TEST(UserLineRenderParams_PreservesLoopsAndDisconnectedSections)
{
  df::UserLineRenderParams params;
  params.AddSpline({{0, 0}, {1, 0}, {0, 0}});
  params.AddSpline({});
  params.AddSpline({{10, 0}, {10, 0}});
  params.AddSpline({{20, 0}, {21, 0}});
  TEST_EQUAL(params.m_splines.size(), 2, ());
  TEST_EQUAL(params.m_splines[0]->GetPath(), (std::vector<m2::PointD>{{0, 0}, {1, 0}, {0, 0}}), ());
  TEST_EQUAL(params.m_splines[1]->GetPath(), (std::vector<m2::PointD>{{20, 0}, {21, 0}}), ());
}
