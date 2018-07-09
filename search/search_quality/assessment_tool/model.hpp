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

protected:
  View * m_view = nullptr;
};
