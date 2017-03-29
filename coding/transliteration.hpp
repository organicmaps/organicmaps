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

  static Transliteration & Instance();

  void Init(std::string const & icuDataDir);

  bool Transliterate(std::string const & str, int8_t langCode, std::string & out) const;

private:
  Transliteration() = default;

  std::map<std::string, std::unique_ptr<icu::Transliterator>> m_transliterators;
};
