#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace om::opening_hours
{
enum class TokenType
{
  Number,      // 0-9
  Colon,       // : colon between hours and minutes
  Dash,        // - dash for time ranges
  EndOfInput,  // end of string
  Invalid      // unrecognized character
};

std::ostream & operator<<(std::ostream & os, const TokenType & type);

struct Token
{
  TokenType type;
  std::string_view value;
  size_t position;
};

class Tokenizer
{
public:
  explicit Tokenizer(const std::string_view & input);
  std::vector<Token> tokenize();

private:
  std::string_view m_input;
  size_t m_position;

  Token readNumber();
  void skipWhitespace();
};
}  // namespace om::opening_hours




