#include "testing/testing.hpp"

#include "opening_hours/tokenizer.hpp"

namespace om::opening_hours
{
UNIT_TEST(Tokenizer_EmptyString)
{
  Tokenizer tokenizer("");
  std::vector<Token> tokens = tokenizer.tokenize();

  TEST_EQUAL(tokens.size(), 1, ());
  TEST_EQUAL(tokens[0].type, TokenType::EndOfInput, ());
}

UNIT_TEST(Tokenizer_SingleTime)
{
  Tokenizer tokenizer("08:30");
  std::vector<Token> tokens = tokenizer.tokenize();

  TEST_EQUAL(tokens.size(), 4, ());

  // 08
  TEST_EQUAL(tokens[0].type, TokenType::Number, ());
  TEST_EQUAL(tokens[0].value, "08", ());

  // :
  TEST_EQUAL(tokens[1].type, TokenType::Colon, ());
  TEST_EQUAL(tokens[1].value, ":", ());

  // 30
  TEST_EQUAL(tokens[2].type, TokenType::Number, ());
  TEST_EQUAL(tokens[2].value, "30", ());
}

UNIT_TEST(Tokenizer_TimeRange)
{
  Tokenizer tokenizer("08:30-17:00");
  std::vector<Token> tokens = tokenizer.tokenize();

  TEST_EQUAL(tokens.size(), 8, ());

  // Check first time: 08:30
  TEST_EQUAL(tokens[0].type, TokenType::Number, ());   // 08
  TEST_EQUAL(tokens[1].type, TokenType::Colon, ());    // :
  TEST_EQUAL(tokens[2].type, TokenType::Number, ());   // 30

  // Check dash
  TEST_EQUAL(tokens[3].type, TokenType::Dash, ());     // -

  // Check second time: 17:00
  TEST_EQUAL(tokens[4].type, TokenType::Number, ());   // 17
  TEST_EQUAL(tokens[5].type, TokenType::Colon, ());    // :
  TEST_EQUAL(tokens[6].type, TokenType::Number, ());   // 00
}

UNIT_TEST(Tokenizer_InvalidInput)
{
  Tokenizer tokenizer("08:3X");
  std::vector<Token> tokens = tokenizer.tokenize();

  // Should handle invalid character 'X'
  TEST_EQUAL(tokens[3].type, TokenType::Invalid, ());
  TEST_EQUAL(tokens[3].value, "X", ());
}

}  // namespace om::opening_hours
