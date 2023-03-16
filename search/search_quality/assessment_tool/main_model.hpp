#pragma once
#include "context.hpp"
#include "edits.hpp"
#include "model.hpp"
#include "search_request_runner.hpp"
#include "view.hpp"

#include "search/feature_loader.hpp"

#include "base/thread_checker.hpp"

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
  void InitiateBackgroundSearch(size_t const from, size_t const to) override;

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
  void FlipSampleUsefulness(int index) override;

private:
  static int constexpr kInvalidIndex = -1;

  void InitiateForegroundSearch(size_t index);

  void OnUpdate(View::ResultType type, size_t sampleIndex, ResultsEdits::Update const & update);
  void OnSampleUpdate(size_t sampleIndex);

  void UpdateViewOnResults(search::Results const & results);
  void ShowMarks(Context const & context);

  void OnChangeAllRelevancesClicked(ResultsEdits::Relevance relevance);

  template <typename Fn>
  void ForAnyMatchingEntry(Context & context, FeatureID const & id, Fn && fn);

  Framework & m_framework;
  DataSource const & m_dataSource;
  search::FeatureLoader m_loader;

  ContextList m_contexts;

  // Path to the last file search samples were loaded from or saved to.
  std::string m_path;

  int m_selectedSample = kInvalidIndex;

  SearchRequestRunner m_runner;

  ThreadChecker m_threadChecker;
};
