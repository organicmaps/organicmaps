#pragma once

#include "std/string.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"

namespace platform
{
// class GetTextById is ready to work with different sources of text strings.
// For the time being it's only strings for TTS.
enum class TextSource
{
  TtsSound = 0
};

/// GetTextById represents text messages which are saved in textsDir
/// in a specified locale.
class GetTextById
{
public:
  GetTextById(TextSource textSouce, string const & localeName);
  /// The constructor is used for writing unit tests only.
  GetTextById(string const & jsonBuffer);

  bool IsValid() const { return !m_localeTexts.empty(); }
  /// @return a pair of a text string in a specified locale for textId and a boolean flag.
  /// If textId is found in m_localeTexts then the boolean flag is set to true.
  /// The boolean flag is set to false otherwise.
  string operator()(string const & textId) const;

private:
  void InitFromJson(string const & jsonBuffer);

  unordered_map<string, string> m_localeTexts;
};
}  // namespace platform
