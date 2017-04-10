#pragma once

#include <string>

class View;

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
  virtual void OnShowViewportClicked() = 0;
  virtual void OnShowPositionClicked() = 0;
  virtual bool HasChanges() = 0;

protected:
  View * m_view = nullptr;
};
