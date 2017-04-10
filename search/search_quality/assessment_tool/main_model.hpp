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
  void Save() override;
  void SaveAs(std::string const & path) override;

  void OnSampleSelected(int index) override;
  void OnResultSelected(int index) override;
  void OnShowViewportClicked() override;
  void OnShowPositionClicked() override;
  bool HasChanges() override;

private:
  static int constexpr kInvalidIndex = -1;

  void OnUpdate(size_t index, Edits::Update const & update);

  void OnResults(uint64_t timestamp, size_t index, search::Results const & results,
                 std::vector<search::Sample::Result::Relevance> const & relevances,
                 std::vector<size_t> const & goldenMatching,
                 std::vector<size_t> const & actualMatching);

  void ResetSearch();

  Framework & m_framework;
  Index const & m_index;

  ContextList m_contexts;

  // Path to the last file search samples were loaded from or saved to.
  std::string m_path;

  std::weak_ptr<search::ProcessorHandle> m_queryHandle;
  uint64_t m_queryTimestamp = 0;
  int m_selectedSample = kInvalidIndex;
  size_t m_numShownResults = 0;

  ThreadChecker m_threadChecker;
};
