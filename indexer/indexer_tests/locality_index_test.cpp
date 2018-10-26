#include "testing/testing.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"
#include "indexer/locality_object.hpp"

#include "coding/file_container.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "base/geo_object_id.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

using namespace indexer;
using namespace std;

namespace
{
struct LocalityObjectVector
{
  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    for_each(m_objects.cbegin(), m_objects.cend(), forward<ToDo>(toDo));
  }

  vector<LocalityObject> m_objects;
};

template <class ObjectsVector, class Writer>
void BuildGeoObjectsIndex(ObjectsVector const & objects, Writer & writer,
                          string const & tmpFilePrefix)
{
  auto coverLocality = [](indexer::LocalityObject const & o, int cellDepth) {
    return covering::CoverGeoObject(o, cellDepth);
  };
  return covering::BuildLocalityIndex<ObjectsVector, Writer, kGeoObjectsDepthLevels>(
      objects, writer, coverLocality, tmpFilePrefix);
}

using Ids = set<uint64_t>;
using RankedIds = vector<uint64_t>;

template <typename LocalityIndex>
Ids GetIds(LocalityIndex const & index, m2::RectD const & rect)
{
  Ids ids;
  index.ForEachInRect([&ids](base::GeoObjectId const & id) { ids.insert(id.GetEncodedId()); },
                      rect);
  return ids;
};

template <typename LocalityIndex>
RankedIds GetRankedIds(LocalityIndex const & index, m2::PointD const & center,
                       m2::PointD const & border, uint32_t topSize)
{
  RankedIds ids;
  index.ForClosestToPoint(
      [&ids](base::GeoObjectId const & id) { ids.push_back(id.GetEncodedId()); }, center,
      MercatorBounds::DistanceOnEarth(center, border), topSize);
  return ids;
};

UNIT_TEST(BuildLocalityIndexTest)
{
  LocalityObjectVector objects;
  objects.m_objects.resize(4);
  objects.m_objects[0].SetForTests(1, m2::PointD{0, 0});
  objects.m_objects[1].SetForTests(2, m2::PointD{1, 0});
  objects.m_objects[2].SetForTests(3, m2::PointD{1, 1});
  objects.m_objects[3].SetForTests(4, m2::PointD{0, 1});

  vector<char> localityIndex;
  MemWriter<vector<char>> writer(localityIndex);
  BuildGeoObjectsIndex(objects, writer, "tmp");
  MemReader reader(localityIndex.data(), localityIndex.size());

  indexer::GeoObjectsIndex<MemReader> index(reader);

  TEST_EQUAL(GetIds(index, m2::RectD{-0.5, -0.5, 0.5, 0.5}), (Ids{1}), ());
  TEST_EQUAL(GetIds(index, m2::RectD{0.5, -0.5, 1.5, 1.5}), (Ids{2, 3}), ());
  TEST_EQUAL(GetIds(index, m2::RectD{-0.5, -0.5, 1.5, 1.5}), (Ids{1, 2, 3, 4}), ());
}

UNIT_TEST(LocalityIndexRankTest)
{
  LocalityObjectVector objects;
  objects.m_objects.resize(4);
  objects.m_objects[0].SetForTests(1, m2::PointD{1, 0});
  objects.m_objects[1].SetForTests(2, m2::PointD{2, 0});
  objects.m_objects[2].SetForTests(3, m2::PointD{3, 0});
  objects.m_objects[3].SetForTests(4, m2::PointD{4, 0});

  vector<char> localityIndex;
  MemWriter<vector<char>> writer(localityIndex);
  BuildGeoObjectsIndex(objects, writer, "tmp");
  MemReader reader(localityIndex.data(), localityIndex.size());

  indexer::GeoObjectsIndex<MemReader> index(reader);
  TEST_EQUAL(GetRankedIds(index, m2::PointD{1, 0} /* center */, m2::PointD{4, 0} /* border */,
                          4 /* topSize */),
             (vector<uint64_t>{1, 2, 3, 4}), ());
  TEST_EQUAL(GetRankedIds(index, m2::PointD{1, 0} /* center */, m2::PointD{3, 0} /* border */,
                          4 /* topSize */),
             (vector<uint64_t>{1, 2, 3}), ());
  TEST_EQUAL(GetRankedIds(index, m2::PointD{4, 0} /* center */, m2::PointD{1, 0} /* border */,
                          4 /* topSize */),
             (vector<uint64_t>{4, 3, 2, 1}), ());
  TEST_EQUAL(GetRankedIds(index, m2::PointD{4, 0} /* center */, m2::PointD{1, 0} /* border */,
                          2 /* topSize */),
             (vector<uint64_t>{4, 3}), ());
  TEST_EQUAL(GetRankedIds(index, m2::PointD{3, 0} /* center */, m2::PointD{0, 0} /* border */,
                          1 /* topSize */),
             (vector<uint64_t>{3}), ());
}

UNIT_TEST(LocalityIndexTopSizeTest)
{
  LocalityObjectVector objects;
  objects.m_objects.resize(7);
  // Same cell.
  objects.m_objects[0].SetForTests(1, m2::PointD{1.0, 0.0});
  objects.m_objects[1].SetForTests(2, m2::PointD{1.0, 0.0});
  objects.m_objects[2].SetForTests(3, m2::PointD{1.0, 0.0});
  objects.m_objects[3].SetForTests(4, m2::PointD{1.0, 0.0});
  // Another close cell.
  objects.m_objects[4].SetForTests(5, m2::PointD{1.0, 1.0});
  objects.m_objects[5].SetForTests(6, m2::PointD{1.0, 1.0});
  // Far cell.
  objects.m_objects[6].SetForTests(7, m2::PointD{10.0, 10.0});

  vector<char> localityIndex;
  MemWriter<vector<char>> writer(localityIndex);
  BuildGeoObjectsIndex(objects, writer, "tmp");
  MemReader reader(localityIndex.data(), localityIndex.size());

  indexer::GeoObjectsIndex<MemReader> index(reader);
  TEST_EQUAL(GetRankedIds(index, m2::PointD{1.0, 0.0} /* center */,
                          m2::PointD{10.0, 10.0} /* border */, 4 /* topSize */)
                 .size(),
             4, ());

  // 4 objects are indexed at the central cell. Index does not guarantee the order but must
  // return 4 objects.
  TEST_EQUAL(GetRankedIds(index, m2::PointD{1.0, 0.0} /* center */,
                          m2::PointD{10.0, 10.0} /* border */, 3 /* topSize */)
                 .size(),
             4, ());

  // At the {1.0, 1.0} point there are also 2 objects, but it's not a central cell, index must
  // return 5 (topSize) objects.
  TEST_EQUAL(GetRankedIds(index, m2::PointD{1.0, 0.0} /* center */,
                          m2::PointD{10.0, 10.0} /* border */, 5 /* topSize */)
                 .size(),
             5, ());

  // The same here. There are not too many objects in central cell. Index must return 5 (topSize)
  // objects.
  TEST_EQUAL(GetRankedIds(index, m2::PointD{4.0, 0.0} /* center */,
                          m2::PointD{10.0, 10.0} /* border */, 5 /* topSize */)
                 .size(),
             5, ());

  TEST_EQUAL(GetRankedIds(index, m2::PointD{4.0, 0.0} /* center */,
                          m2::PointD{10.0, 10.0} /* border */, 7 /* topSize */)
                 .size(),
             7, ());
}
}  // namespace
