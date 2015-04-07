#pragma once

#include "std/string.hpp"
#include "std/map.hpp"


class StringsBundle
{
  typedef map<string, string> TStringMap;
  TStringMap m_values;
  TStringMap m_defValues;

public:
  void SetDefaultString(string const & name, string const & value);
  void SetString(string const & name, string const & value);
  string const GetString(string const & name) const;
};
