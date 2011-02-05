#include "../tesselator.hpp"
#include "../geometry_serialization.hpp"
#include "../mercator.hpp"

#include "../../coding/reader.hpp"
#include "../../coding/writer.hpp"

#include "../../testing/testing.hpp"


namespace
{
  typedef m2::PointD P;

  bool is_equal(P const & p1, P const & p2)
  {
    return p1.EqualDxDy(p2, MercatorBounds::GetCellID2PointAbsEpsilon());
  }

  bool FindTriangle(serial::OutPointsT const & test, P arr[])
  {
    size_t const count = test.size();
    for (size_t i = 0; i < count; i+=3)
    {
      for (int base = 0; base < 3; ++base)
        if (is_equal(test[i], arr[base]) &&
            is_equal(test[i+1], arr[(base+1)%3]) &&
            is_equal(test[i+2], arr[(base+2)%3]))
        {
          return true;
        }
    }
    return false;
  }

  void CompareTriangles(serial::OutPointsT const & test,
                        P arrP[], uintptr_t arrT[][3], size_t count)
  {
    TEST_EQUAL(test.size(), 3*count, (test));

    for (size_t i = 0; i < count; ++i)
    {
      P trg[] = { arrP[arrT[i][0]], arrP[arrT[i][1]], arrP[arrT[i][2]] };
      TEST ( FindTriangle(test, trg), ("Triangles : ", test, " Etalon : ", trg[0], trg[1], trg[2]) );
    }
  }

  void TestTrianglesCoding(P arrP[], size_t countP, uintptr_t arrT[][3], size_t countT)
  {
    tesselator::TrianglesInfo info;
    info.AssignPoints(arrP, arrP + countP);

    info.Reserve(countT);

    for (size_t i = 0; i < countT; ++i)
      info.Add(arrT[i]);

    serial::TrianglesChainSaver saver(0);

    tesselator::PointsInfo points;
    info.GetPointsInfo(saver.GetBasePoint(), saver.GetMaxPoint(), &serial::pts::D2U, points);

    info.ProcessPortions(points, saver);

    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    saver.Save(writer);

    TEST ( !buffer.empty(), () );

    MemReader reader(&buffer[0], buffer.size());
    ReaderSource<MemReader> src(reader);

    serial::OutPointsT triangles;
    serial::LoadOuterTriangles(src, 0, triangles);

    CompareTriangles(triangles, arrP, arrT, countT);
  }
}

UNIT_TEST(TrianglesCoding_Smoke)
{
  {
    P arrP[] =  { P(0, 0), P(0, 1), P(1, 0), P(1, 1), P(0, -1), P(-1, 0) };
    uintptr_t arrT[][3] = { {0, 1, 2}, {1, 3, 2}, {4, 0, 2}, {1, 0, 5}, {4, 5, 0} };

    TestTrianglesCoding(arrP, ARRAY_SIZE(arrP), arrT, ARRAY_SIZE(arrT));
  }
}
