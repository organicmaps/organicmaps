#pragma once

#include "../base/buffer_vector.hpp"

namespace gui
{
class CountryStatusHelper
{
public:
  enum ControlType
  {
    Label,
    Button,
    Progress
  };

  struct Control
  {
    string m_label;
    ControlType m_type;
  };

  CountryStatusHelper();

  size_t GetComponentCount() const;
  Control const & GetControl(size_t index) const;
  float GetControlMargin() const;

  void GetProgressInfo(string & alphabet, size_t & maxLength);
  string GetProgressValue() const;

  int GetState() const;
  bool IsVisibleForState(int state) const;

private:
  buffer_vector<Control, 4> m_controls;
};

}  // namespace gui
