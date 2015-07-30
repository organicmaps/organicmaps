#pragma once

#include <std/string.hpp>
#include <std/unordered_map.hpp>

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

  bool IsValid() const { return !m_localeTexts.empty(); }
  string operator()(string const & textId) const;

private:
  unordered_map<string, string> m_localeTexts;
};
}  // namespace platform
