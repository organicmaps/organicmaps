#pragma once

#include <map>
#include <memory>
#include <string>

namespace icu
{
class Transliterator;
}

class Transliteration
{
public:
  ~Transliteration();

  static Transliteration & GetInstance();

  void Init(std::string const & icuDataDir);

  std::string Transliterate(std::string const & str, int8_t langCode) const;

private:
  Transliteration() = default;

  struct TransliteratorWrapper;
  std::map<std::string, std::unique_ptr<icu::Transliterator>> m_transliterators;
};
