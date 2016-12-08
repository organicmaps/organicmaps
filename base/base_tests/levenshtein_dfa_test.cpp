#include "testing/testing.hpp"

#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"

#include "std/string.hpp"

using namespace strings;

namespace
{
enum class Status
{
  Accepts,
  Rejects,
  Intermediate
};

Status GetStatus(LevenshteinDFA const & dfa, string const & s)
{
  auto it = dfa.Begin();
  DFAMove(it, s);
  if (it.Accepts())
    return Status::Accepts;
  if (it.Rejects())
    return Status::Rejects;
  return Status::Intermediate;
}

bool Accepts(LevenshteinDFA const & dfa, string const & s)
{
  return GetStatus(dfa, s) == Status::Accepts;
}

bool Rejects(LevenshteinDFA const & dfa, string const & s)
{
  return GetStatus(dfa, s) == Status::Rejects;
}

bool Intermediate(LevenshteinDFA const & dfa, string const & s)
{
  return GetStatus(dfa, s) == Status::Intermediate;
}

UNIT_TEST(LevenshteinDFA_Smoke)
{
  {
    LevenshteinDFA dfa("", 0 /* maxErrors */);

    auto it = dfa.Begin();
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    it.Move('a');
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());

    it.Move('b');
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());
  }

  {
    LevenshteinDFA dfa("abc", 1 /* maxErrors */);

    TEST(Accepts(dfa, "ab"), ());
    TEST(Accepts(dfa, "abd"), ());
    TEST(Accepts(dfa, "abcd"), ());
    TEST(Accepts(dfa, "bbc"), ());

    TEST(Rejects(dfa, "cba"), ());
    TEST(Rejects(dfa, "abcde"), ());

    TEST(Accepts(dfa, "ac"), ());
    TEST(Accepts(dfa, "acb"), ());
    TEST(Accepts(dfa, "acbc"), ());
    TEST(Rejects(dfa, "acbd"), ());

    TEST(Intermediate(dfa, "a"), ());
  }

  {
    LevenshteinDFA dfa("ленинградский", 2 /* maxErrors */);

    TEST(Accepts(dfa, "ленинградский"), ());
    TEST(Accepts(dfa, "ленингадский"), ());
    TEST(Accepts(dfa, "ленигнрадский"), ());
    TEST(Rejects(dfa, "ленинский"), ());
  }

  {
    LevenshteinDFA dfa("atm", 1 /* maxErrors */);
    TEST(Rejects(dfa, "san"), ());
  }
}

UNIT_TEST(LevenshteinDFA_Prefix)
{
  {
    LevenshteinDFA dfa("москва", 1 /* prefixCharsToKeep */, 1 /* maxErrors */);
    TEST(Accepts(dfa, "москва"), ());
    TEST(Accepts(dfa, "масква"), ());
    TEST(Accepts(dfa, "моска"), ());
    TEST(Rejects(dfa, "иосква"), ());
  }
  {
    LevenshteinDFA dfa("москва", 0 /* prefixCharsToKeep */, 1 /* maxErrors */);
    TEST(Accepts(dfa, "москва"), ());
    TEST(Accepts(dfa, "иосква"), ());
    TEST(Accepts(dfa, "моксва"), ());
  }
}
}  // namespace
