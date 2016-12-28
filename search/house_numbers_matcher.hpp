#pragma once

#include "base/string_utils.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

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
  Token(strings::UniString && value, Type type) : m_value(move(value)), m_type(type) {}
  Token(Token &&) = default;

  Token & operator=(Token &&) = default;
  Token & operator=(Token const &) = default;

  bool operator==(Token const & rhs) const
  {
    return m_type == rhs.m_type && m_value == rhs.m_value;
  }

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

// Tokenizes |s| that may be a house number.
void Tokenize(strings::UniString s, bool isPrefix, vector<Token> & ts);

// Parses a string that can be one or more house numbers. This method
// can be used to parse addr:housenumber fields.
void ParseHouseNumber(strings::UniString const & s, vector<vector<Token>> & parses);

// Parses a part of search query that can be a house number.
void ParseQuery(strings::UniString const & query, bool queryIsPrefix, vector<Token> & parse);

// Returns true if house number matches to a given query.
bool HouseNumbersMatch(strings::UniString const & houseNumber, strings::UniString const & query,
                       bool queryIsPrefix);

// Returns true if house number matches to a given parsed query.
bool HouseNumbersMatch(strings::UniString const & houseNumber, vector<Token> const & queryParse);

// Returns true if |s| looks like a house number.
bool LooksLikeHouseNumber(strings::UniString const & s, bool isPrefix);

string DebugPrint(Token::Type type);

string DebugPrint(Token const & token);
}  // namespace house_numbers
}  // namespace search
