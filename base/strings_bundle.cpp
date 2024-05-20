#include "base/strings_bundle.hpp"

void StringsBundle::SetDefaultString(std::string const & name, std::string const & value)
{
  m_defValues[name] = value;
}

void StringsBundle::SetString(std::string const & name, std::string const & value)
{
  m_values[name] = value;
}

std::string const StringsBundle::GetString(std::string const & name) const
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
