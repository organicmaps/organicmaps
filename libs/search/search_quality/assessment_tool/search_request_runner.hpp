#pragma once

#include "search/search_quality/assessment_tool/context.hpp"

#include "map/framework.hpp"

#include "base/thread_checker.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <vector>

// A proxy for SearchAPI/SearchEngine.
// This class updates the Model's |m_contexts| directly (from the main thread) and updates
// the View via the |m_updateViewOnResults| and |m_updateSampleSearchState| callbacks.
class SearchRequestRunner
{
public:
  using UpdateViewOnResults = std::function<void(search::Results const & results)>;
  using UpdateSampleSearchState = std::function<void(size_t index)>;

  SearchRequestRunner(Framework & framework, DataSource const & dataSource, ContextList & contexts,
                      UpdateViewOnResults && updateViewOnResults, UpdateSampleSearchState && updateSampleSearchState);

  void InitiateForegroundSearch(size_t index);

  void InitiateBackgroundSearch(size_t from, size_t to);

  void ResetForegroundSearch();

  void ResetBackgroundSearch();

private:
  static size_t constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

  // Tries to run the unprocessed request with the smallest index, if there is one.
  void RunNextBackgroundRequest(size_t timestamp);

  void RunRequest(size_t index, bool background, size_t timestamp);

  void PrintBackgroundSearchStats() const;

  Framework & m_framework;

  DataSource const & m_dataSource;

  ContextList & m_contexts;

  UpdateViewOnResults m_updateViewOnResults;
  UpdateSampleSearchState m_updateSampleSearchState;

  std::weak_ptr<search::ProcessorHandle> m_foregroundQueryHandle;
  std::map<size_t, std::weak_ptr<search::ProcessorHandle>> m_backgroundQueryHandles;

  size_t m_foregroundTimestamp = 0;
  size_t m_backgroundTimestamp = 0;

  size_t m_backgroundFirstIndex = kInvalidIndex;
  size_t m_backgroundLastIndex = kInvalidIndex;
  std::queue<size_t> m_backgroundQueue;
  size_t m_backgroundNumProcessed = 0;

  ThreadChecker m_threadChecker;
};
