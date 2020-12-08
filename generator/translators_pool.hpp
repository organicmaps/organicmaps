#pragma once

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/translator_interface.hpp"

#include "base/thread_pool_computational.hpp"
#include "base/thread_safe_queue.hpp"

#include <memory>
#include <vector>

namespace generator
{
class TranslatorsPool
{
public:
  explicit TranslatorsPool(std::shared_ptr<TranslatorInterface> const & original,
                           size_t threadCount);

  void Emit(std::vector<OsmElement> elements);
  bool Finish();

private:
  base::thread_pool::computational::ThreadPool m_threadPool;
  base::threads::ThreadSafeQueue<std::shared_ptr<TranslatorInterface>> m_translators;
};
}  // namespace generator
