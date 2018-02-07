#include "testing/testing.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"
#include "indexer/locality_object.hpp"

#include "coding/file_container.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "base/osm_id.hpp"

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

using Ids = set<uint64_t>;

template <typename LocalityIndex>
Ids GetIds(LocalityIndex const & index, m2::RectD const & rect)
{
  Ids ids;
  index.ForEachInRect([&ids](osm::Id const & id) { ids.insert(id.EncodedId()); }, rect);
  return ids;
};

UNIT_TEST(LocalityIndexTest)
{
  LocalityObjectVector objects;
  objects.m_objects.resize(4);
  objects.m_objects[0].SetForTests(1, m2::PointD{0, 0});
  objects.m_objects[1].SetForTests(2, m2::PointD{1, 0});
  objects.m_objects[2].SetForTests(3, m2::PointD{1, 1});
  objects.m_objects[3].SetForTests(4, m2::PointD{0, 1});

  vector<char> localityIndex;
  MemWriter<vector<char>> writer(localityIndex);
  covering::BuildLocalityIndex(objects, writer, "tmp");
  MemReader reader(localityIndex.data(), localityIndex.size());

  indexer::LocalityIndex<MemReader> index(reader);

  TEST_EQUAL(GetIds(index, m2::RectD{-0.5, -0.5, 0.5, 0.5}), (Ids{1}), ());
  TEST_EQUAL(GetIds(index, m2::RectD{0.5, -0.5, 1.5, 1.5}), (Ids{2, 3}), ());
  TEST_EQUAL(GetIds(index, m2::RectD{-0.5, -0.5, 1.5, 1.5}), (Ids{1, 2, 3, 4}), ());
}
}  // namespace
