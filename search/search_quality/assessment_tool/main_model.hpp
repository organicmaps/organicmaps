#pragma once

#include "search/engine.hpp"
#include "search/search_quality/assessment_tool/context.hpp"
#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/assessment_tool/model.hpp"
#include "search/search_quality/sample.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <vector>
#include <memory>

class Framework;
class Index;

namespace search
{
class Results;
}

class MainModel : public Model
{
public:
  explicit MainModel(Framework & framework);

  // Model overrides:
  void Open(std::string const & path) override;
  void OnSampleSelected(int index) override;

private:
  void OnUpdate(size_t index);

  void OnResults(uint64_t timestamp, size_t index, search::Results const & results,
                 std::vector<search::Sample::Result::Relevance> const & relevances);

  void ResetSearch();

  Framework & m_framework;
  Index const & m_index;
  ContextList m_contexts;

  bool m_loading = false;

  std::weak_ptr<search::ProcessorHandle> m_queryHandle;
  uint64_t m_queryTimestamp = 0;
  size_t m_numShownResults = 0;

  ThreadChecker m_threadChecker;
};
