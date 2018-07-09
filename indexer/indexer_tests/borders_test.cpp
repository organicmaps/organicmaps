#include "testing/testing.hpp"

#include "indexer/borders.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <vector>

using namespace indexer;
using namespace std;

namespace
{
struct BordersVector
{
  struct Border
  {
    uint64_t m_id;
    vector<m2::PointD> m_outer;
    vector<vector<m2::PointD>> m_inners;
  };

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    for (auto const & border : m_borders)
      toDo(border.m_id, border.m_outer, border.m_inners);
  }

  vector<Border> m_borders;
};

UNIT_TEST(BordersTest)
{
  {
    BordersVector vec;
    vec.m_borders.resize(1);
    vec.m_borders[0].m_id = 0;
    vec.m_borders[0].m_outer = {m2::PointD{0, 0}, m2::PointD{1, 0}, m2::PointD{1, 1}, m2::PointD{0, 1}};

    indexer::Borders borders;
    borders.DeserializeFromVec(vec);

    TEST(borders.IsPointInside(0, m2::PointD{0.5, 0.5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{-0.5, 0.5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{-0.5, -0.5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{-0.5, -0.5}), ());
  }
  {
    BordersVector vec;
    vec.m_borders.resize(2);
    vec.m_borders[0].m_id = 0;
    vec.m_borders[0].m_outer = {m2::PointD{0, 0}, m2::PointD{1, 0}, m2::PointD{1, 1}, m2::PointD{0, 1}};
    vec.m_borders[1].m_id = 0;
    vec.m_borders[1].m_outer = {m2::PointD{2, 2}, m2::PointD{3, 2}, m2::PointD{3, 3}, m2::PointD{2, 3}};

    indexer::Borders borders;
    borders.DeserializeFromVec(vec);

    TEST(borders.IsPointInside(0, m2::PointD{0.5, 0.5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{0.5, 2.5}), ());
    TEST(borders.IsPointInside(0, m2::PointD{2.5, 2.5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{2.5, 0.5}), ());
  }
  {
    BordersVector vec;
    vec.m_borders.resize(2);
    vec.m_borders[0].m_id = 0;
    vec.m_borders[0].m_outer = {m2::PointD{0, 0}, m2::PointD{1, 0}, m2::PointD{1, 1}, m2::PointD{0, 1}};
    vec.m_borders[1].m_id = 1;
    vec.m_borders[1].m_outer = {m2::PointD{2, 2}, m2::PointD{3, 2}, m2::PointD{3, 3}, m2::PointD{2, 3}};

    indexer::Borders borders;
    borders.DeserializeFromVec(vec);

    TEST(borders.IsPointInside(0, m2::PointD{0.5, 0.5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{2.5, 2.5}), ());
    TEST(borders.IsPointInside(1, m2::PointD{2.5, 2.5}), ());
    TEST(!borders.IsPointInside(1, m2::PointD{0.5, 0.5}), ());
  }
  {
    BordersVector vec;
    vec.m_borders.resize(1);
    vec.m_borders[0].m_id = 0;
    vec.m_borders[0].m_outer = {m2::PointD{0, 0}, m2::PointD{10, 0}, m2::PointD{10, 10}, m2::PointD{0, 10}};
    vec.m_borders[0].m_inners = {{m2::PointD{2, 2}, m2::PointD{8, 2}, m2::PointD{8, 8}, m2::PointD{2, 8}}};

    indexer::Borders borders;
    borders.DeserializeFromVec(vec);

    TEST(borders.IsPointInside(0, m2::PointD{1, 1}), ());
    TEST(borders.IsPointInside(0, m2::PointD{9, 9}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{3, 7}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{5, 5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{6, 6}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{7, 3}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{7, 7}), ());
  }
  {
    BordersVector vec;
    vec.m_borders.resize(1);
    vec.m_borders[0].m_id = 0;
    vec.m_borders[0].m_outer = {m2::PointD{0, 0}, m2::PointD{10, 0}, m2::PointD{10, 10}, m2::PointD{0, 10}};
    vec.m_borders[0].m_inners = {{m2::PointD{2, 2}, m2::PointD{5, 2}, m2::PointD{5, 5}, m2::PointD{2, 5}},
                                 {m2::PointD{5, 5}, m2::PointD{8, 5}, m2::PointD{8, 8}, m2::PointD{5, 8}}};

    indexer::Borders borders;
    borders.DeserializeFromVec(vec);

    TEST(borders.IsPointInside(0, m2::PointD{1, 1}), ());
    TEST(borders.IsPointInside(0, m2::PointD{9, 9}), ());
    TEST(borders.IsPointInside(0, m2::PointD{3, 7}), ());
    TEST(borders.IsPointInside(0, m2::PointD{7, 3}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{5, 5}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{6, 6}), ());
    TEST(!borders.IsPointInside(0, m2::PointD{7, 7}), ());
  }
}
}  // namespace
