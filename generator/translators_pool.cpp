#include "generator/translators_pool.hpp"

#include <future>

namespace generator
{
TranslatorsPool::TranslatorsPool(std::shared_ptr<TranslatorInterface> const & original,
                                 std::shared_ptr<cache::IntermediateData> const & cache,
                                 size_t threadCount)
  : m_threadPool(threadCount)
{
  m_translators.Push(original);
  for (size_t i = 0; i < threadCount; ++i)
    m_translators.Push(original->Clone());

  CHECK_EQUAL(m_translators.size(), threadCount + 1, ());
}

void TranslatorsPool::Emit(std::vector<OsmElement> && elements)
{
  std::shared_ptr<TranslatorInterface> translator;
  m_translators.WaitAndPop(translator);
  m_threadPool.SubmitWork([&, translator, elements{move(elements)}]() mutable {
    for (auto & element : elements)
      translator->Emit(element);

    m_translators.Push(translator);
  });
}

bool TranslatorsPool::Finish()
{
  m_threadPool.WaitingStop();
  using TranslatorPtr = std::shared_ptr<TranslatorInterface>;
  base::threads::ThreadSafeQueue<std::future<TranslatorPtr>> queue;
  while (!m_translators.Empty())
  {
    std::promise<TranslatorPtr> p;
    std::shared_ptr<TranslatorInterface> translator;
    m_translators.TryPop(translator);
    p.set_value(translator);
    queue.Push(p.get_future());
  }

  base::thread_pool::computational::ThreadPool pool(queue.size() / 2 + 1);
  while (queue.Size() != 1)
  {
    std::future<TranslatorPtr> left;
    std::future<TranslatorPtr> right;
    queue.WaitAndPop(left);
    queue.WaitAndPop(right);
    queue.Push(pool.Submit([left{move(left)}, right{move(right)}]() mutable {
      auto leftTranslator = left.get();
      auto rigthTranslator = right.get();
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
