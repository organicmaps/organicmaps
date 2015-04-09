#include "testing/testing.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/scales.hpp"

#include "base/logging.hpp"


UNIT_TEST(VisibleScales_Smoke)
{
  classificator::Load();

  {
    char const * arr[] = { "place", "city", "capital" };
    uint32_t const type = classif().GetTypeByPath(vector<string>(arr, arr + 3));

    pair<int, int> const r = feature::GetDrawableScaleRange(type);
    TEST_NOT_EQUAL(r.first, -1, ());
    TEST_LESS_OR_EQUAL(r.first, r.second, ());

    TEST(my::between_s(r.first, r.second, 10), (r));
    TEST(!my::between_s(r.first, r.second, 1), (r));
    TEST(!my::between_s(r.first, r.second, scales::GetUpperScale()), (r));
  }
}

namespace
{

class DoGetMaxLowMinHighZoom
{
  pair<int, int> m_res;
  string m_low;

  set<uint32_t> m_skip;

public:
  DoGetMaxLowMinHighZoom(Classificator const & c) : m_res(-1, 1000)
  {
    char const * arr[][2] = {
      { "highway", "proposed" },
      { "highway", "bus_stop" },
    };

    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
      m_skip.insert(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
  }

  void operator() (ClassifObject const * p, uint32_t type)
  {
    if (m_skip.count(type) > 0)
      return;

    pair<int, int> const r = feature::GetDrawableScaleRange(type);
    ASSERT(r.first != -1 && r.second != -1, (r));

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

}

UNIT_TEST(VisibleScales_Highway)
{
  Classificator const & c = classif();

  char const * arr[] = { "highway" };
  uint32_t const type = c.GetTypeByPath(vector<string>(arr, arr + 1));

  ClassifObject const * pObj = c.GetObject(type);

  DoGetMaxLowMinHighZoom doGet(c);
  pObj->ForEachObjectInTree(doGet, type);

  doGet.Print();
}
