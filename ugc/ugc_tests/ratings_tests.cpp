#include "testing/testing.hpp"

#include "ugc/types.hpp"

#include "base/math.hpp"

#include <cstdint>
#include <utility>
#include <vector>

using namespace std;
using namespace ugc;

namespace
{
UNIT_TEST(PackRating_Smoke)
{
  vector<uint32_t> basedOn;
  for (uint32_t count = 0; count < 100; ++count)
    basedOn.push_back(count);

  vector<float> ratings;
  for (int r = 0; r <= 100; ++r)
    ratings.push_back(static_cast<float>(r) / 10.0f);

  for (auto const b : basedOn)
  {
    for (auto const r : ratings)
    {
      UGC ugc;
      ugc.m_basedOn = b;
      ugc.m_totalRating = r;

      auto const packed = ugc.GetPackedRating();
      auto const unpacked = UGC::UnpackRating(packed);

      if (ugc.m_basedOn == 0)
        TEST_EQUAL(unpacked.first, 0, ());
      else if (ugc.m_basedOn <= 2)
        TEST_EQUAL(unpacked.first, 1, ());
      else if (ugc.m_basedOn <= 10)
        TEST_EQUAL(unpacked.first, 2, ());
      else
        TEST_EQUAL(unpacked.first, 3, ());

      if (ugc.m_totalRating <= 4.0)
      {
        TEST_EQUAL(unpacked.second, 4.0, ());
      }
      else
      {
        float constexpr kEps = 0.1f;
        TEST(base::AlmostEqualAbs(ugc.m_totalRating, unpacked.second, kEps),
             (ugc.m_totalRating, unpacked.second));
      }
    }
  }
}
}  // namespace
