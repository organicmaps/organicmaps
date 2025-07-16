#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

// From ICU library, either 3party/icu or from the system's package.
#include <unicode/uversion.h>

U_NAMESPACE_BEGIN
class UnicodeString;
U_NAMESPACE_END

class Transliteration
{
public:
  enum class Mode
  {
    Enabled,
    Disabled
  };

  ~Transliteration();

  static Transliteration & Instance();

  void Init(std::string const & icuDataDir);

  void SetMode(Mode mode);
  // Transliterates |sv| with transliterators set for |langCode| in StringUtf8Multilang
  // if mode is set to Enabled.
  bool Transliterate(std::string_view sv, int8_t langCode, std::string & out) const;

  // Transliterates |str| with |transliteratorId|, ignores mode.
  bool TransliterateForce(std::string const & str, std::string const & transliteratorId, std::string & out) const;

private:
  struct TransliteratorInfo;

  Transliteration();

  bool Transliterate(std::string_view transID, icu::UnicodeString & ustr) const;

  std::mutex m_initializationMutex;
  std::atomic<bool> m_inited;
  std::atomic<Mode> m_mode;
  std::map<std::string_view, std::unique_ptr<TransliteratorInfo>> m_transliterators;
};
