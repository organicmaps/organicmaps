#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace geocoder
{
class NameDictionary
{
public:
  // Values of Position type: kUnspecifiedPosition or >= 1.
  using Position = std::uint32_t;

  static constexpr Position kUnspecifiedPosition = 0;

  NameDictionary() = default;
  NameDictionary(NameDictionary &&) = default;
  NameDictionary & operator=(NameDictionary &&) = default;

  NameDictionary(NameDictionary const &) = delete;
  NameDictionary & operator=(NameDictionary const &) = delete;

  std::string const & Get(Position position) const;
  Position Add(std::string const & s);

private:
  std::vector<std::string> m_stock;
};

class NameDictionaryMaker
{
public:
  NameDictionaryMaker() = default;
  NameDictionaryMaker(NameDictionaryMaker const &) = delete;
  NameDictionaryMaker & operator=(NameDictionaryMaker const &) = delete;

  NameDictionary::Position Add(std::string const & s);
  NameDictionary Release();

private:
  NameDictionary m_dictionary;
  std::unordered_map<std::string, NameDictionary::Position> m_index;
};
}  // namespace geocoder
