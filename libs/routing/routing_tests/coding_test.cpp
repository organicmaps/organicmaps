#include "testing/testing.hpp"

#include "routing/coding.hpp"

#include <cstdint>
#include <limits>
#include <type_traits>

using namespace routing;

namespace
{
template <typename T, typename ToDo>
void ForEachNumber(ToDo && toDo)
{
  for (T number = std::numeric_limits<T>::min();; ++number)
  {
    toDo(number);

    if (number == std::numeric_limits<T>::max())
      break;
  }
}

template <typename T>
void TestZigZag(T prev, T current)
{
  auto const delta = EncodeZigZagDelta(prev, current);
  auto const decodedCurrent = DecodeZigZagDelta(prev, delta);
  TEST_EQUAL(decodedCurrent, current, ("prev:", prev, "delta:", delta));
}

template <typename T>
void TestZigZagUnsigned()
{
  static_assert(std::is_unsigned<T>::value, "T should be an unsigned type");

  constexpr auto max = std::numeric_limits<T>::max();
  constexpr T values[] = {0, 1, 7, max / 2, max - 1, max};
  for (T prev : values)
    for (T current : values)
      TestZigZag(prev, current);
}

template <typename T>
void TestZigZagSigned()
{
  static_assert(std::is_signed<T>::value, "T should be a signed type");

  constexpr auto min = std::numeric_limits<T>::min();
  constexpr auto max = std::numeric_limits<T>::max();
  constexpr T values[] = {min, min + 1, min / 2, -7, -1, 0, 1, 7, max / 2, max - 1, max};
  for (T prev : values)
    for (T current : values)
      TestZigZag(prev, current);
}
}  // namespace

namespace routing_test
{
UNIT_TEST(ModuleCastTest)
{
  ForEachNumber<uint8_t>([](uint8_t number)
  {
    auto signedNumber = ModularCast(number);
    static_assert(std::is_same<decltype(signedNumber), int8_t>::value, "int8_t expected");
    TEST_EQUAL(static_cast<uint8_t>(signedNumber), number, ("signedNumber:", signedNumber));
  });
}

UNIT_TEST(ZigZagUint8)
{
  ForEachNumber<uint8_t>([](uint8_t prev)
  { ForEachNumber<uint8_t>([&](uint8_t current) { TestZigZag(prev, current); }); });
}

UNIT_TEST(ZigZagInt8)
{
  ForEachNumber<int8_t>([](int8_t prev) { ForEachNumber<int8_t>([&](int8_t current) { TestZigZag(prev, current); }); });
}

UNIT_TEST(ZigZagUint16)
{
  TestZigZagUnsigned<uint16_t>();
}
UNIT_TEST(ZigZagInt16)
{
  TestZigZagSigned<int16_t>();
}
UNIT_TEST(ZigZagUint32)
{
  TestZigZagUnsigned<uint32_t>();
}
UNIT_TEST(ZigZagInt32)
{
  TestZigZagSigned<int32_t>();
}
UNIT_TEST(ZigZagUint64)
{
  TestZigZagUnsigned<uint64_t>();
}
UNIT_TEST(ZigZagInt64)
{
  TestZigZagSigned<int64_t>();
}
}  // namespace routing_test
