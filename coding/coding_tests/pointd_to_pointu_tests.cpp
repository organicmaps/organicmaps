#include "testing/testing.hpp"

#include "coding/pointd_to_pointu.hpp"

#include "base/logging.hpp"

#include <cmath>

using namespace std;

namespace
{
uint32_t const kCoordBits = POINT_COORD_BITS;
}  // namespace

UNIT_TEST(PointDToPointU_Epsilons)
{
  m2::PointD const arrPt[] = {{-180, -180}, {-180, 180}, {180, 180}, {180, -180}};
  m2::PointD const arrD[] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
  size_t const count = ARRAY_SIZE(arrPt);

  double eps = 1.0;
  while (true)
  {
    size_t i = 0;
    for (; i < count; ++i)
    {
      m2::PointU p0 = PointDToPointU(arrPt[i].x, arrPt[i].y, kCoordBits);
      m2::PointU p1 = PointDToPointU(arrPt[i].x + arrD[i].x * eps,
                                    arrPt[i].y + arrD[i].y * eps,
                                    kCoordBits);

      if (p0 != p1)
        break;
    }
    if (i == count)
      break;

    eps *= 0.1;
  }

  LOG(LINFO, ("Epsilon (relative error) =", eps));

  for (size_t i = 0; i < count; ++i)
  {
    m2::PointU const p1 = PointDToPointU(arrPt[i].x, arrPt[i].y, kCoordBits);
    m2::PointU const p2(p1.x + arrD[i].x, p1.y + arrD[i].y);
    m2::PointD const p3 = PointUToPointD(p2, kCoordBits);

    LOG(LINFO, ("Dx =", p3.x - arrPt[i].x, "Dy =", p3.y - arrPt[i].y));
  }
}
