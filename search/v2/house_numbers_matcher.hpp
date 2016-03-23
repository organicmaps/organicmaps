#pragma once

#include "base/string_utils.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
namespace v2
{
// This class splits a string representing a house number to groups of
// symbols from the same class (separators, digits or other symbols,
// hope, letters).
class HouseNumberTokenizer
{
public:
  enum class CharClass
  {
    Separator,
    Digit,
    Other,
  };

  struct Token
  {
    Token() : m_klass(CharClass::Separator) {}
    Token(strings::UniString const & token, CharClass klass) : m_token(token), m_klass(klass) {}
    Token(strings::UniString && token, CharClass klass) : m_token(move(token)), m_klass(klass) {}

    strings::UniString m_token;
    CharClass m_klass;
  };

  // Performs greedy split of |s| by character classes. Note that this
  // function never emits Tokens corresponding to Separator classes.
  static void Tokenize(strings::UniString const & s, vector<Token> & ts);
};

// Splits house number by tokens, removes blanks and separators.
void NormalizeHouseNumber(strings::UniString const & s, vector<strings::UniString> & ts);

// Returns true when |query| matches to |houseNumber|.
bool HouseNumbersMatch(strings::UniString const & houseNumber, strings::UniString const & query);

// Returns true when |queryTokens| match to |houseNumber|.
bool HouseNumbersMatch(strings::UniString const & houseNumber,
                       vector<strings::UniString> const & queryTokens);

string DebugPrint(HouseNumberTokenizer::CharClass charClass);

string DebugPrint(HouseNumberTokenizer::Token const & token);
}  // namespace v2
}  // namespace search
