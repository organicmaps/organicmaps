#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

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
  bool Transliterate(std::string const & str, int8_t langCode, std::string & out) const;

private:
  Transliteration();

  std::atomic<Mode> m_mode;

  struct TransliteratorInfo;
  std::map<std::string, std::unique_ptr<TransliteratorInfo>> m_transliterators;
};
