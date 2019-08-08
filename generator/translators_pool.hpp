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
                           std::shared_ptr<generator::cache::IntermediateData> const & cache,
                           size_t copyCount);

  void Emit(std::vector<OsmElement> && elements);
  bool Finish();

private:
  std::vector<std::shared_ptr<TranslatorInterface>> m_translators;
  base::thread_pool::computational::ThreadPool m_threadPool;
  base::threads::ThreadSafeQueue<base::threads::DataWrapper<size_t>> m_freeTranslators;
};
}  // namespace generator
