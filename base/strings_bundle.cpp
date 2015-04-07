#include "base/strings_bundle.hpp"


void StringsBundle::SetDefaultString(string const & name, string const & value)
{
  m_defValues[name] = value;
}

void StringsBundle::SetString(string const & name, string const & value)
{
  m_values[name] = value;
}

string const StringsBundle::GetString(string const & name) const
{
  TStringMap::const_iterator it = m_values.find(name);
  if (it != m_values.end())
    return it->second;
  else
  {
    it = m_defValues.find(name);
    if (it != m_defValues.end())
      return it->second;
  }
  return "";
}
