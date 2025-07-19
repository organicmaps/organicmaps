#include "tokenizer.hpp"
#include <cctype>

namespace om::opening_hours
{

Tokenizer::Tokenizer(const std::string_view & input)
  : m_input(input), m_position(0)
{
}

void Tokenizer::skipWhitespace()
{
  while (m_position < m_input.size() && std::isspace(m_input[m_position]))
    m_position++;
}

Token Tokenizer::readNumber()
{
  size_t start = m_position;
  while (m_position < m_input.size() && std::isdigit(m_input[m_position]))
    m_position++;
  std::string_view numberValue = m_input.substr(start, m_position - start);
  return {TokenType::Number, numberValue, start};
}

std::ostream & operator<<(std::ostream & os, const TokenType & type)
{
  switch (type)
  {
    case TokenType::Number:     return os << "Number";
    case TokenType::Colon:      return os << "Colon";
    case TokenType::Dash:       return os << "Dash";
    case TokenType::EndOfInput: return os << "EndOfInput";
    case TokenType::Invalid:    return os << "Invalid";
    default:                    return os << "Unknown";
  }
}

std::vector<Token> Tokenizer::tokenize()
{
  std::vector<Token> tokens;
  const size_t inputLength = m_input.size();

  while (m_position < inputLength)
  {
    // skip whitespace
    skipWhitespace();

    // check for end of string
    if (m_position >= inputLength)
      break;

    // check for numbers
    if (std::isdigit(m_input[m_position]))
    {
      tokens.push_back(readNumber());
      continue;
    }

    if (m_input[m_position] == ':')
    {
      size_t start = m_position;
      tokens.push_back({TokenType::Colon, m_input.substr(m_position, 1), start});
      m_position++;
      continue;
    }

    if (m_input[m_position] == '-')
    {
      size_t start = m_position;
      tokens.push_back({TokenType::Dash, m_input.substr(m_position, 1), start});
      m_position++;
      continue;
    }

    tokens.push_back(Token{TokenType::Invalid, m_input.substr(m_position, 1), m_position});
    m_position += 1;
  }

  // end of input token
  tokens.push_back({TokenType::EndOfInput, "", m_position});

  return tokens;
}

}  // namespace om::opening_hours