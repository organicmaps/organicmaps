#pragma once

#include <map>
#include <string>

class StringsBundle
{
  using TStringMap = std::map<std::string, std::string>;
  TStringMap m_values;
  TStringMap m_defValues;

public:
  void SetDefaultString(std::string const & name, std::string const & value);
  void SetString(std::string const & name, std::string const & value);
  std::string const GetString(std::string const & name) const;
};
