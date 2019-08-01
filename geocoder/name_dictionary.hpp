#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace geocoder
{
class MultipleNames
{
public:
  using const_iterator = std::vector<std::string>::const_iterator;

  explicit MultipleNames(std::string const & mainName = {});

  std::string const & GetMainName() const noexcept;
  std::vector<std::string> const & GetNames() const noexcept;

  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;

  void SetMainName(std::string const & name);
  // Complexity: O(N-1) - a best case, O(N*log(N)) - a worst case.
  void AddAltName(std::string const & name);

  friend bool operator==(MultipleNames const & lhs, MultipleNames const & rhs) noexcept;
  friend bool operator!=(MultipleNames const & lhs, MultipleNames const & rhs) noexcept;

private:
  std::vector<std::string> m_names;
};

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

  MultipleNames const & Get(Position position) const;
  Position Add(MultipleNames && s);

private:
  std::vector<MultipleNames> m_stock;
};

class NameDictionaryBuilder
{
public:
  NameDictionaryBuilder() = default;
  NameDictionaryBuilder(NameDictionaryBuilder const &) = delete;
  NameDictionaryBuilder & operator=(NameDictionaryBuilder const &) = delete;

  NameDictionary::Position Add(MultipleNames && s);
  NameDictionary Release();

private:
  struct Hash
  {
    size_t operator()(MultipleNames const & names) const noexcept;
  };

  NameDictionary m_dictionary;
  std::unordered_map<MultipleNames, NameDictionary::Position, Hash> m_index;
};
}  // namespace geocoder
