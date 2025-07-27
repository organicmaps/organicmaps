#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace
{
class DoGetMaxLowMinHighZoom
{
  Classificator const & m_classif;
  pair<int, int> m_res;
  string m_low;

  set<uint32_t> m_skip;
  bool IsSkip(uint32_t t) const
  {
    ftype::TruncValue(t, 2);
    return m_skip.count(t) > 0;
  }

public:
  explicit DoGetMaxLowMinHighZoom(Classificator const & c) : m_classif(classif()), m_res(-1, 1000)
  {
    char const * arr[][2] = {
        {"highway", "bus_stop"},
        {"highway", "speed_camera"},
        {"highway", "world_level"},
        {"highway", "world_towns_level"},
    };

    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
      m_skip.insert(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
  }

  void operator()(ClassifObject const * p, uint32_t type)
  {
    if (IsSkip(type))
      return;

    pair<int, int> const r = feature::GetDrawableScaleRange(type);
    if (r.first == -1 || r.second == -1)
    {
      LOG(LINFO, (r, m_classif.GetFullObjectName(type)));
      return;
    }

    if (m_res.first < r.first)
    {
      m_res.first = r.first;
      m_low = p->GetName();
    }
    if (m_res.second > r.second)
      m_res.second = r.second;
  }

  void Print()
  {
    TEST_EQUAL(m_res.second, scales::GetUpperStyleScale(), (m_res));
    LOG(LINFO, ("Max low highway zoom:", m_res, "for type:", m_low));
  }
};

}  // namespace

UNIT_TEST(VisibleScales_Highway)
{
  Classificator const & c = classif();

  char const * arr[] = {"highway"};
  uint32_t const type = c.GetTypeByPath(vector<string>(arr, arr + 1));

  ClassifObject const * pObj = c.GetObject(type);

  DoGetMaxLowMinHighZoom doGet(c);
  pObj->ForEachObjectInTree(doGet, type);

  doGet.Print();
}
