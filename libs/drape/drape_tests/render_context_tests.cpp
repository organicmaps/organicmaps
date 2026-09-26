#include "drape/render_context.hpp"
#include "testing/testing.hpp"

#include <thread>
#include <vector>

namespace render_context_tests
{
struct State
{
  int m_value = 0;
};

UNIT_TEST(RenderContext_IndependentViewsAndNestedScopes)
{
  auto first = std::make_shared<dp::RenderContext>();
  auto second = std::make_shared<dp::RenderContext>();
  {
    dp::RenderContext::Scope scope(first);
    dp::RenderContext::Get<State>().m_value = 16;
    {
      dp::RenderContext::Scope nested(second);
      TEST_EQUAL(dp::RenderContext::Get<State>().m_value, 0, ());
      dp::RenderContext::Get<State>().m_value = 18;
    }
    TEST_EQUAL(dp::RenderContext::Get<State>().m_value, 16, ());
  }
  {
    dp::RenderContext::Scope scope(second);
    TEST_EQUAL(dp::RenderContext::Get<State>().m_value, 18, ());
    second->Clear();
    TEST_EQUAL(dp::RenderContext::Get<State>().m_value, 0, ());
  }
  dp::RenderContext::Scope scope(first);
  TEST_EQUAL(dp::RenderContext::Get<State>().m_value, 16, ());
  first->Clear();
}

UNIT_TEST(RenderContext_WorkersShareOnlyTheirOwnView)
{
  auto first = std::make_shared<dp::RenderContext>();
  auto second = std::make_shared<dp::RenderContext>();
  {
    dp::RenderContext::Scope scope(first);
    dp::RenderContext::Get<State>().m_value = 1;
  }
  {
    dp::RenderContext::Scope scope(second);
    dp::RenderContext::Get<State>().m_value = 2;
  }
  std::vector<std::thread> workers;
  std::atomic<bool> valid{true};
  for (int i = 0; i < 8; ++i)
    workers.emplace_back([&, i]
    {
      auto context = i % 2 == 0 ? first : second;
      dp::RenderContext::Scope scope(context);
      for (int j = 0; j < 1000; ++j)
        if (dp::RenderContext::Get<State>().m_value != (i % 2 + 1))
          valid = false;
    });
  for (auto & worker : workers)
    worker.join();
  TEST(valid.load(), ());
}
}  // namespace render_context_tests
