#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace icu
{
class UnicodeString;
}  // namespace icu

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
  // Transliterates |str| with transliterators set for |langCode| in StringUtf8Multilang
  // if mode is set to Enabled.
  bool Transliterate(std::string const & str, int8_t langCode, std::string & out) const;

  // Transliterates |str| with |transliteratorId|, ignores mode.
  bool TransliterateForce(std::string const & str, std::string const & transliteratorId,
                          std::string & out) const;

private:
  struct TransliteratorInfo;

  Transliteration();

  bool Transliterate(std::string transliteratorId, icu::UnicodeString & ustr) const;

  std::mutex m_initializationMutex;
  std::atomic<bool> m_inited;
  std::atomic<Mode> m_mode;
  std::map<std::string, std::unique_ptr<TransliteratorInfo>> m_transliterators;
};
