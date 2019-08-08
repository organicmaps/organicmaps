#include "generator/translators_pool.hpp"

#include <future>

namespace generator
{
TranslatorsPool::TranslatorsPool(std::shared_ptr<TranslatorInterface> const & original,
                                 std::shared_ptr<cache::IntermediateData> const & cache,
                                 size_t copyCount)
  : m_translators({original})
  , m_threadPool(copyCount + 1)
{
  m_freeTranslators.Push(0);
  m_translators.reserve(copyCount + 1);
  for (size_t i = 0; i < copyCount; ++i)
  {
    auto cache_ = cache->Clone();
    auto translator = original->Clone(cache_);
    m_translators.emplace_back(translator);
    m_freeTranslators.Push(i + 1);
  }
}

void TranslatorsPool::Emit(std::vector<OsmElement> && elements)
{
  base::threads::DataWrapper<size_t> d;
  m_freeTranslators.WaitAndPop(d);
  auto const idx = d.Get();
  m_threadPool.SubmitWork([&, idx, elements{move(elements)}]() mutable {
    for (auto & element : elements)
      m_translators[idx]->Emit(element);

    m_freeTranslators.Push(idx);
  });
}

bool TranslatorsPool::Finish()
{
  m_threadPool.WaitAndStop();
  using TranslatorPtr = std::shared_ptr<TranslatorInterface>;
  base::threads::ThreadSafeQueue<std::future<TranslatorPtr>> queue;
  for (auto const & t : m_translators)
  {
    std::promise<TranslatorPtr> p;
    p.set_value(t);
    queue.Push(p.get_future());
  }

  base::thread_pool::computational::ThreadPool pool(m_translators.size() / 2 + 1);
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
