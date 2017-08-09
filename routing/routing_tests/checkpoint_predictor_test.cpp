#include "testing/testing.hpp"

#include "routing/checkpoint_predictor.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <vector>

using namespace routing;
using namespace std;

namespace
{
double constexpr kEpsMeters = 1.0;

void TestDistanceBetweenPoints(vector<m2::PointD> const & points, double expectedLengthMeters)
{
  double const d = DistanceBetweenPointsMeters(points);
  TEST(my::AlmostEqualAbs(d, expectedLengthMeters, kEpsMeters), (d, expectedLengthMeters));
}

UNIT_TEST(DistanceBetweenPointsMetersTest)
{
  TestDistanceBetweenPoints({} /* points */, 0.0 /* expectedLengthMeters */);

  TestDistanceBetweenPoints({{0.0, 0.0}} /* points */, 0.0 /* expectedLengthMeters */);

  TestDistanceBetweenPoints({{0.0, 0.0}, {1.0, 0.0}} /* points */,
                            111317.0 /* expectedLengthMeters */);

  TestDistanceBetweenPoints({{0.0, 0.0}, {0.5, 0.0}, {0.5, 0.5}, {0.5, 0.0}, {1.0, 0.0}} /* points */,
                            222633.0 /* expectedLengthMeters */);
}

UNIT_TEST(CheckpointPredictorSmokeTest)
{
  CheckpointPredictor checkpointPredictor({0.0, 0.0} /* start */, {1.0, 0.0} /* finish */);
  TEST(checkpointPredictor({}).empty(), ());
  // Single intermediate point case. It's always between the start and the finish.
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{0.5, 0.0}})), vector<int8_t>({0}), ());
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{-0.5, 0.0}})), vector<int8_t>({0}), ());
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{1.5, 0.0}})), vector<int8_t>({0}), ());
}

UNIT_TEST(CheckpointPredictorTest)
{
  CheckpointPredictor checkpointPredictor({0.0, 0.0} /* start */, {4.0, 0.0} /* finish */);
  // Several intermediate point case. Intermediate point with index zero is an added intermediate point.
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{1.0, 0.5}, {2.0, -0.5}})),
             vector<int8_t>({0, 1}), ());
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{2.0, 0.5}, {1.0, -0.5}})),
             vector<int8_t>({1, 0}), ());

  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{1.0, 0.5}, {2.0, -0.5}, {3.0, 0.5}})),
             vector<int8_t>({0, 1, 2}), ());
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{2.0, 0.5}, {1.0, -0.5}, {3.0, 0.5}})),
             vector<int8_t>({1, 0, 2}), ());
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{3.0, 0.5}, {0.0, -0.5}, {1.0, 0.5}})),
             vector<int8_t>({2, 0, 1}), ());
  TEST_EQUAL(checkpointPredictor(vector<m2::PointD>({{3.0, 0.5}, {1.0, -0.5}, {0.0, 0.5}})),
             vector<int8_t>({2, 0, 1}), ());
}
}  // namespace
