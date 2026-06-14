#include "testing/testing.hpp"

#include "drape/hw_texture.hpp"

#include <cstdint>
#include <vector>

namespace
{
UNIT_TEST(Mipmap_LevelsCount)
{
  TEST_EQUAL(dp::GetMipLevelsCount(16, 16), 5, ());  // 16,8,4,2,1
  TEST_EQUAL(dp::GetMipLevelsCount(1, 1), 1, ());
  TEST_EQUAL(dp::GetMipLevelsCount(8, 4), 4, ());  // 8x4,4x2,2x1,1x1
  TEST_EQUAL(dp::GetMipLevelsCount(256, 1), 9, ());
}

UNIT_TEST(Mipmap_BuildLevels_DimsAndCount)
{
  std::vector<uint8_t> const lvl0(static_cast<size_t>(16) * 16 * 4, 0);
  auto const levels = dp::BuildMipmapLevels(lvl0.data(), 16, 16, 4);

  // Levels 1..N-1 (excludes level 0).
  TEST_EQUAL(levels.size(), dp::GetMipLevelsCount(16, 16) - 1, ());
  uint32_t const expected[] = {8, 4, 2, 1};
  for (size_t i = 0; i < levels.size(); ++i)
  {
    TEST_EQUAL(levels[i].m_width, expected[i], (i));
    TEST_EQUAL(levels[i].m_height, expected[i], (i));
    TEST_EQUAL(levels[i].m_data.size(), static_cast<size_t>(expected[i]) * expected[i] * 4, (i));
  }
}

// The crux: a fully transparent texel's RGB must not bleed into the downsampled colour, otherwise the
// hatch darkens at coarse mips (gray halo). One opaque white texel + three transparent ones must average
// to white RGB with quarter alpha - not to gray.
UNIT_TEST(Mipmap_BuildLevels_AlphaWeightedNoHalo)
{
  // 2x2 RGBA: top-left opaque white, rest fully transparent with zero RGB.
  std::vector<uint8_t> lvl0 = {
      255, 255, 255, 255, /*TL*/ 0, 0, 0, 0, /*TR*/
      0,   0,   0,   0,   /*BL*/ 0, 0, 0, 0, /*BR*/
  };
  auto const levels = dp::BuildMipmapLevels(lvl0.data(), 2, 2, 4);
  TEST_EQUAL(levels.size(), 1, ());
  auto const & px = levels[0].m_data;
  TEST_EQUAL(px.size(), 4, ());
  TEST_EQUAL(px[0], 255, ("R must stay white, not gray"));
  TEST_EQUAL(px[1], 255, ("G must stay white, not gray"));
  TEST_EQUAL(px[2], 255, ("B must stay white, not gray"));
  TEST_EQUAL(px[3], 64, ("alpha is the plain average: (255+0+0+0+2)/4"));
}

UNIT_TEST(Mipmap_BuildLevels_SingleChannelPlainAverage)
{
  // 2x2 single-channel (e.g. coverage mask): plain box average, no alpha weighting.
  std::vector<uint8_t> const lvl0 = {200, 100, 50, 10};
  auto const levels = dp::BuildMipmapLevels(lvl0.data(), 2, 2, 1);
  TEST_EQUAL(levels.size(), 1, ());
  TEST_EQUAL(levels[0].m_data.size(), 1, ());
  TEST_EQUAL(levels[0].m_data[0], static_cast<uint8_t>((200 + 100 + 50 + 10 + 2) / 4), ());  // 90
}
}  // namespace
