#pragma once

#include "search/engine.hpp"
#include "search/feature_loader.hpp"
#include "search/search_quality/assessment_tool/context.hpp"
#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/assessment_tool/model.hpp"
#include "search/search_quality/assessment_tool/view.hpp"
#include "search/search_quality/sample.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <vector>
#include <memory>

#include <boost/optional.hpp>

class Framework;
class DataSource;

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
  void OnNonFoundResultSelected(int index) override;
  void OnShowViewportClicked() override;
  void OnShowPositionClicked() override;
  void OnMarkAllAsRelevantClicked() override;
  void OnMarkAllAsIrrelevantClicked() override;
  bool HasChanges() override;
  bool AlreadyInSamples(FeatureID const & id) override;
  void AddNonFoundResult(FeatureID const & id) override;

private:
  static int constexpr kInvalidIndex = -1;

  void OnUpdate(View::ResultType type, size_t sampleIndex, Edits::Update const & update);

  void OnResults(uint64_t timestamp, size_t sampleIndex, search::Results const & results,
                 std::vector<boost::optional<Edits::Relevance>> const & relevances,
                 std::vector<size_t> const & goldenMatching,
                 std::vector<size_t> const & actualMatching);

  void ResetSearch();
  void ShowMarks(Context const & context);

  void OnChangeAllRelevancesClicked(Edits::Relevance relevance);

  template <typename Fn>
  void ForAnyMatchingEntry(Context & context, FeatureID const & id, Fn && fn);

  Framework & m_framework;
  DataSource const & m_dataSource;
  search::FeatureLoader m_loader;

  ContextList m_contexts;

  // Path to the last file search samples were loaded from or saved to.
  std::string m_path;

  std::weak_ptr<search::ProcessorHandle> m_queryHandle;
  uint64_t m_queryTimestamp = 0;
  int m_selectedSample = kInvalidIndex;
  size_t m_numShownResults = 0;

  ThreadChecker m_threadChecker;
};
