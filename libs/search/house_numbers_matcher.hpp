#pragma once

#include "indexer/feature_utils.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <utility>
#include <vector>

namespace search
{
namespace house_numbers
{
struct Token
{
  enum Type
  {
    TYPE_NUMBER,
    TYPE_SEPARATOR,
    TYPE_GROUP_SEPARATOR,
    TYPE_HYPHEN,
    TYPE_SLASH,
    TYPE_STRING,
    TYPE_BUILDING_PART,
    TYPE_LETTER,
    TYPE_BUILDING_PART_OR_LETTER
  };

  Token() = default;
  Token(strings::UniString const & value, Type type) : m_value(value), m_type(type) {}
  Token(strings::UniString && value, Type type) : m_value(std::move(value)), m_type(type) {}
  Token(Token &&) = default;

  Token & operator=(Token &&) = default;
  Token & operator=(Token const &) = default;

  bool operator==(Token const & rhs) const { return m_type == rhs.m_type && m_value == rhs.m_value; }

  bool operator!=(Token const & rhs) const { return !(*this == rhs); }

  bool operator<(Token const & rhs) const
  {
    if (m_type != rhs.m_type)
      return m_type < rhs.m_type;
    return m_value < rhs.m_value;
  }

  strings::UniString m_value;
  Type m_type = TYPE_SEPARATOR;
  bool m_prefix = false;
};

using TokensT = std::vector<Token>;

// Used to convert Token::Type::TYPE_NUMBER into int value.
uint64_t ToUInt(strings::UniString const & s);

// Tokenizes |s| that may be a house number.
void Tokenize(strings::UniString s, bool isPrefix, TokensT & ts);

// Parses a string that can be one or more house numbers. This method
// can be used to parse addr:housenumber fields.
void ParseHouseNumber(strings::UniString const & s, std::vector<TokensT> & parses);

// Parses a part of search query that can be a house number.
void ParseQuery(strings::UniString const & query, bool queryIsPrefix, TokensT & parse);

/// @return true if house number matches to a given parsed query.
/// @{
bool HouseNumbersMatch(strings::UniString const & houseNumber, TokensT const & queryParse);
bool HouseNumbersMatchConscription(strings::UniString const & houseNumber, TokensT const & queryParse);
bool HouseNumbersMatchRange(std::string_view const & hnRange, TokensT const & queryParse,
                            feature::InterpolType interpol);
/// @}

// Returns true if |s| looks like a house number.
bool LooksLikeHouseNumber(strings::UniString const & s, bool isPrefix);
bool LooksLikeHouseNumber(std::string const & s, bool isPrefix);

bool LooksLikeHouseNumberStrict(strings::UniString const & s);
bool LooksLikeHouseNumberStrict(std::string const & s);

std::string DebugPrint(Token::Type type);

std::string DebugPrint(Token const & token);
}  // namespace house_numbers
}  // namespace search
