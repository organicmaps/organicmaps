#include "testing/testing.hpp"

#include "base/dfa_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/uni_string_dfa.hpp"

using namespace strings;

namespace
{
UNIT_TEST(UniStringDFA_Smoke)
{
  {
    UniStringDFA dfa("");

    auto it = dfa.Begin();
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "a");
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());
  }

  {
    UniStringDFA dfa("абв");

    auto it = dfa.Begin();
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "а");
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "б");
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "в");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "г");
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());
  }

  {
    UniStringDFA dfa("абв");

    auto it = dfa.Begin();
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "а");
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "г");
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());
  }
}

UNIT_TEST(UniStringDFA_Prefix)
{
  {
    PrefixDFAModifier<UniStringDFA> dfa(UniStringDFA("abc"));

    auto it = dfa.Begin();
    DFAMove(it, "ab");

    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "c");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "d");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    DFAMove(it, "efghijk");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());
  }
}
}  // namespace
