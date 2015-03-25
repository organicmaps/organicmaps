#include "country_status_helper.hpp"
#include "drape_gui.hpp"

#include "../base/string_utils.hpp"

namespace gui
{
CountryStatusHelper::CountryStatusHelper()
{
  // TODO UVR
  m_controls.push_back(Control{"Belarus", Label});
  m_controls.push_back(Control{"Download map\n70MB", Button});
  m_controls.push_back(Control{"Download map + routing\n70MB", Button});
  m_controls.push_back(Control{"", Progress});
}

size_t CountryStatusHelper::GetComponentCount() const { return m_controls.size(); }

CountryStatusHelper::Control const & CountryStatusHelper::GetControl(size_t index) const
{
  return m_controls[index];
}

float CountryStatusHelper::GetControlMargin()
{
  return 5.0f * DrapeGui::Instance().GetScaleFactor();
}

void CountryStatusHelper::GetProgressInfo(string & alphabet, size_t & maxLength)
{
  alphabet = " 0123456789%";
  maxLength = 5;
}

string CountryStatusHelper::GetProgressValue() const
{
  // TODO UVR
  static int value = 0;
  static int counter = 0;
  if (counter >= 60)
  {
    value++;
    counter = 0;
  }
  counter++;

  return strings::to_string(value) + "%";
}

int CountryStatusHelper::GetState() const
{
  // TODO UVR
  return 0;
}

bool CountryStatusHelper::IsVisibleForState(int state) const
{
  // TODO UVR
  return state != 0;
}

}  // namespace gui
