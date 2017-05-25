#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>

class Transliteration
{
public:
  ~Transliteration();

  static Transliteration & Instance();

  void Init(std::string const & icuDataDir);

  void SetEnabled(bool enable);
  bool Transliterate(std::string const & str, int8_t langCode, std::string & out) const;

private:
  Transliteration();

  std::atomic<bool> m_enabled;

  struct TransliteratorInfo;
  std::map<std::string, std::unique_ptr<TransliteratorInfo>> m_transliterators;
};
