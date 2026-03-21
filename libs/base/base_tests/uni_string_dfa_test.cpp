#include "testing/testing.hpp"

#include "base/dfa_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/uni_string_dfa.hpp"

namespace
{
UNIT_TEST(UniStringDFA_Smoke)
{
  {
    strings::UniStringDFA dfa("");

    auto it = dfa.Begin();
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "a");
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());
  }

  {
    strings::UniStringDFA dfa("абв");

    auto it = dfa.Begin();
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "а");
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "б");
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "в");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "г");
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());
  }

  {
    strings::UniStringDFA dfa("абв");

    auto it = dfa.Begin();
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "а");
    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "г");
    TEST(!it.Accepts(), ());
    TEST(it.Rejects(), ());
  }
}

UNIT_TEST(UniStringDFA_Prefix)
{
  {
    strings::PrefixDFAModifier<strings::UniStringDFA> dfa(strings::UniStringDFA("abc"));

    auto it = dfa.Begin();
    strings::DFAMove(it, "ab");

    TEST(!it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "c");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "d");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());

    strings::DFAMove(it, "efghijk");
    TEST(it.Accepts(), ());
    TEST(!it.Rejects(), ());
  }
}
}  // namespace
