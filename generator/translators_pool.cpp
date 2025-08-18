#include "generator/translators_pool.hpp"

#include <future>

namespace generator
{
TranslatorsPool::TranslatorsPool(std::shared_ptr<TranslatorInterface> const & original, size_t threadCount)
  : m_threadPool(threadCount)
{
  CHECK_GREATER_OR_EQUAL(threadCount, 1, ());

  m_translators.Push(original);
  for (size_t i = 1; i < threadCount; ++i)
    m_translators.Push(original->Clone());
}

void TranslatorsPool::Emit(std::vector<OsmElement> && elements)
{
  std::shared_ptr<TranslatorInterface> translator;
  m_translators.WaitAndPop(translator);
  m_threadPool.SubmitWork([&, translator, elements = std::move(elements)]() mutable
  {
    for (auto const & element : elements)
      translator->Emit(element);

    m_translators.Push(translator);
  });
}

bool TranslatorsPool::Finish()
{
  m_threadPool.WaitingStop();
  using TranslatorPtr = std::shared_ptr<TranslatorInterface>;
  threads::ThreadSafeQueue<std::future<TranslatorPtr>> queue;
  while (!m_translators.Empty())
  {
    std::promise<TranslatorPtr> p;
    std::shared_ptr<TranslatorInterface> translator;
    m_translators.TryPop(translator);
    p.set_value(translator);
    queue.Push(p.get_future());
  }

  base::ComputationalThreadPool pool(queue.Size() / 2 + 1);
  CHECK_GREATER_OR_EQUAL(queue.Size(), 1, ());
  while (queue.Size() != 1)
  {
    std::future<TranslatorPtr> left;
    std::future<TranslatorPtr> right;
    queue.WaitAndPop(left);
    queue.WaitAndPop(right);

    struct State
    {
      std::future<TranslatorPtr> left;
      std::future<TranslatorPtr> right;
    };

    // Should be copyable to workaround MSVC bug (https://developercommunity.visualstudio.com/t/108672)
    auto state = std::make_shared<State>();
    state->left = std::move(left);
    state->right = std::move(right);

    queue.Push(pool.Submit([state = std::move(state)]() mutable
    {
      auto leftTranslator = state->left.get();
      auto rigthTranslator = state->right.get();
      rigthTranslator->Finish();
      leftTranslator->Finish();
      leftTranslator->Merge(*rigthTranslator);
      return leftTranslator;
    }));
  }

  std::future<TranslatorPtr> translatorFuture;
  queue.WaitAndPop(translatorFuture);
  auto translator = translatorFuture.get();
  translator->Finish();
  return translator->Save();
}
}  // namespace generator
