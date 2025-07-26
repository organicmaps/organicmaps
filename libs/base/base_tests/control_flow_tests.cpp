#include "testing/testing.hpp"

#include "base/control_flow.hpp"

#include <cstdint>

using namespace base;

namespace
{
struct Repeater
{
  explicit Repeater(uint32_t repetitions) : m_repetitions(repetitions) {}

  template <typename Fn>
  void ForEach(Fn && fn)
  {
    ControlFlowWrapper<Fn> wrapper(std::forward<Fn>(fn));
    for (uint32_t i = 0; i < m_repetitions; ++i)
    {
      ++m_calls;
      if (wrapper() == ControlFlow::Break)
        return;
    }
  }

  uint32_t m_repetitions = 0;
  uint32_t m_calls = 0;
};
}  // namespace

UNIT_TEST(ControlFlow_Smoke)
{
  {
    Repeater repeater(10);
    uint32_t c = 0;
    repeater.ForEach([&c] { ++c; });

    TEST_EQUAL(c, 10, ());
    TEST_EQUAL(c, repeater.m_repetitions, ());
    TEST_EQUAL(c, repeater.m_calls, ());
  }

  {
    Repeater repeater(10);
    uint32_t c = 0;
    repeater.ForEach([&c]
    {
      ++c;
      if (c == 5)
        return ControlFlow::Break;
      return ControlFlow::Continue;
    });

    TEST_EQUAL(c, 5, ());
    TEST_EQUAL(c, repeater.m_calls, ());
  }
}
