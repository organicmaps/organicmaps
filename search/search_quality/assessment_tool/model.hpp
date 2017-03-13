#pragma once

#include <string>

class View;

class Model
{
public:
  virtual ~Model() = default;

  void SetView(View & view) { m_view = &view; }

  virtual void Open(std::string const & path) = 0;
  virtual void OnSampleSelected(int index) = 0;

protected:
  View * m_view = nullptr;
};
