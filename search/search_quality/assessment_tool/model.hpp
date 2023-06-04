#pragma once

#include <string>

class View;
struct FeatureID;

class Model
{
public:
  virtual ~Model() = default;

  void SetView(View & view) { m_view = &view; }

  virtual void Open(std::string const & path) = 0;
  virtual void Save() = 0;
  virtual void SaveAs(std::string const & path) = 0;

  // Initiates the search in the background on all samples
  // in the 1-based range [|from|, |to|], both ends included.
  // Another background search that may currently be running will be cancelled
  // but the results for already completed requests will not be discarded.
  //
  // Does nothing if the range is invalid.
  virtual void InitiateBackgroundSearch(size_t from, size_t to) = 0;

  virtual void OnSampleSelected(int index) = 0;
  virtual void OnResultSelected(int index) = 0;
  virtual void OnNonFoundResultSelected(int index) = 0;
  virtual void OnShowViewportClicked() = 0;
  virtual void OnShowPositionClicked() = 0;
  virtual void OnMarkAllAsRelevantClicked() = 0;
  virtual void OnMarkAllAsIrrelevantClicked() = 0;
  virtual bool HasChanges() = 0;

  virtual bool AlreadyInSamples(FeatureID const & id) = 0;
  virtual void AddNonFoundResult(FeatureID const & id) = 0;
  virtual void FlipSampleUsefulness(int index) = 0;

protected:
  View * m_view = nullptr;
};
