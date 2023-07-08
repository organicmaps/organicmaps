#include "testing/testing.hpp"

#include "base/checked_cast.hpp"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicitly-unsigned-literal"
#endif  // #ifdef __clang__
UNIT_TEST(IsCastValid)
{
  {
    int8_t value = -1;
    TEST(base::IsCastValid<int8_t>(value), ());
    TEST(base::IsCastValid<int16_t>(value), ());
    TEST(base::IsCastValid<int32_t>(value), ());
    TEST(base::IsCastValid<int64_t>(value), ());

    TEST(!base::IsCastValid<uint8_t>(value), ());
    TEST(!base::IsCastValid<uint16_t>(value), ());
    TEST(!base::IsCastValid<uint32_t>(value), ());
    TEST(!base::IsCastValid<uint64_t>(value), ());
  }
  {
    int64_t value = -1;
    TEST(base::IsCastValid<int8_t>(value), ());
    TEST(base::IsCastValid<int16_t>(value), ());
    TEST(base::IsCastValid<int32_t>(value), ());
    TEST(base::IsCastValid<int64_t>(value), ());

    TEST(!base::IsCastValid<uint8_t>(value), ());
    TEST(!base::IsCastValid<uint16_t>(value), ());
    TEST(!base::IsCastValid<uint32_t>(value), ());
    TEST(!base::IsCastValid<uint64_t>(value), ());
  }
  {
    uint8_t value = 128;
    TEST(!base::IsCastValid<int8_t>(value), ());
    TEST(base::IsCastValid<int16_t>(value), ());
    TEST(base::IsCastValid<int32_t>(value), ());
    TEST(base::IsCastValid<int64_t>(value), ());

    TEST(base::IsCastValid<uint8_t>(value), ());
    TEST(base::IsCastValid<uint16_t>(value), ());
    TEST(base::IsCastValid<uint32_t>(value), ());
    TEST(base::IsCastValid<uint64_t>(value), ());
  }
  {
    uint64_t value = 9223372036854775808ULL;
    TEST(!base::IsCastValid<int8_t>(value), ());
    TEST(!base::IsCastValid<int16_t>(value), ());
    TEST(!base::IsCastValid<int32_t>(value), ());
    TEST(!base::IsCastValid<int64_t>(value), ());

    TEST(!base::IsCastValid<uint8_t>(value), ());
    TEST(!base::IsCastValid<uint16_t>(value), ());
    TEST(!base::IsCastValid<uint32_t>(value), ());
    TEST(base::IsCastValid<uint64_t>(value), ());
  }
  {
    int64_t value = -9223372036854775807LL;
    TEST(!base::IsCastValid<int8_t>(value), ());
    TEST(!base::IsCastValid<int16_t>(value), ());
    TEST(!base::IsCastValid<int32_t>(value), ());
    TEST(base::IsCastValid<int64_t>(value), ());

    TEST(!base::IsCastValid<uint8_t>(value), ());
    TEST(!base::IsCastValid<uint16_t>(value), ());
    TEST(!base::IsCastValid<uint32_t>(value), ());
    TEST(!base::IsCastValid<uint64_t>(value), ());
  }
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // #ifdef __clang__
