#pragma once

#include <std/string.hpp>
#include <std/unordered_map.hpp>


namespace platform
{

/// GetTextById represents text messages which are saved in textsDir
/// in a specified locale.
class GetTextById
{
public:
  GetTextById(string const & textsDir, string const & localeName);

  bool IsValid() const { return !m_localeTexts.empty(); }
  string operator()(string const & textId) const;

private:
  unordered_map<string, string> m_localeTexts;
};
}  // namespace platform
