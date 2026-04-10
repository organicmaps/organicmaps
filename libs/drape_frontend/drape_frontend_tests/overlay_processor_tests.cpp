#include "testing/testing.hpp"

#include "drape_frontend/overlay_processor.hpp"

#include <random>

namespace overlay_processor_test
{
using df::OverlayProcessor;

m2::SharedSpline MakeSpline(std::vector<m2::PointD> && pts)
{
  return m2::SharedSpline(std::move(pts));
}

// Helper: check that merged result contains a chain with the given points (in order).
bool HasChain(std::vector<std::vector<m2::PointD>> const & chains, std::vector<m2::PointD> const & expected)
{
  for (auto const & chain : chains)
  {
    if (chain.size() != expected.size())
      continue;
    bool match = true;
    for (size_t i = 0; i < chain.size(); ++i)
    {
      if (!chain[i].EqualDxDy(expected[i], 1e-5))
      {
        match = false;
        break;
      }
    }
    if (match)
      return true;
  }
  return false;
}

// Two connected segments: A->B->C should merge into one chain.
UNIT_TEST(OverlayProcessor_MergeSplines_Simple)
{
  auto const ab = MakeSpline({{0, 0}, {1, 0}});
  auto const bc = MakeSpline({{1, 0}, {2, 0}});

  auto chains = OverlayProcessor::MergeSplines({bc, ab});
  TEST_EQUAL(chains.size(), 1, ());
  TEST_EQUAL(chains[0].size(), 3, ());
  // Oriented left-to-right.
  TEST(chains[0].front().EqualDxDy({0, 0}, 1e-5), ());
  TEST(chains[0].back().EqualDxDy({2, 0}, 1e-5), ());
}

// Disconnected segments: A->B and C->D (no shared endpoint).
UNIT_TEST(OverlayProcessor_MergeSplines_Disconnected)
{
  auto const ab = MakeSpline({{0, 0}, {1, 0}});
  auto const cd = MakeSpline({{5, 0}, {6, 0}});

  auto chains = OverlayProcessor::MergeSplines({ab, cd});
  TEST_EQUAL(chains.size(), 2, ());
}

// Three segments meeting at one point (T-junction):
//   A->J, J->B, J->C
// Should merge A-J-B or A-J-C (one chain of 2), leaving the other as separate.
UNIT_TEST(OverlayProcessor_MergeSplines_TJunction)
{
  m2::PointD const J(1, 0);
  auto const aJ = MakeSpline({{0, 0}, J});
  auto const jB = MakeSpline({J, {2, 0}});  // continues straight
  auto const jC = MakeSpline({J, {1, 1}});  // turns up

  // 3 features at a T-junction produce 2 chains (one gets 2 features, one stays alone).
  // For aJ or jB seed, doesn't matter.

  {
    auto chains = OverlayProcessor::MergeSplines({aJ, jB, jC});
    TEST_EQUAL(chains.size(), 2, ());
    TEST(HasChain(chains, {{0, 0}, {1, 0}, {2, 0}}), ());
    TEST(HasChain(chains, {{1, 0}, {1, 1}}), ());
  }
  {
    auto chains = OverlayProcessor::MergeSplines({jB, jC, aJ});
    TEST_EQUAL(chains.size(), 2, ());
    TEST(HasChain(chains, {{0, 0}, {1, 0}, {2, 0}}), ());
    TEST(HasChain(chains, {{1, 0}, {1, 1}}), ());
  }
}

/// Dual carriageway that splits and re-merges:
///
///          ---<--B_upper--- * ---<--C_upper---
///  ---A---|                                   |---D---
///          --->--B_lower--- * --->--C_lower---
///
UNIT_TEST(OverlayProcessor_MergeSplines_DualCarriageway)
{
  // Shared road before split.
  auto const A = MakeSpline({{0, 0}, {1, 0}});

  // Upper carriageway goes RIGHT-TO-LEFT (opposite to A's direction).
  auto const B_upper = MakeSpline({{2, 1}, {1, 0}});

  // Lower carriageway goes LEFT-TO-RIGHT (same direction as A).
  auto const B_lower = MakeSpline({{1, 0}, {2, -1}});

  // Continue
  auto const C_upper = MakeSpline({{2, 1}, {3, 0}});
  auto const C_lower = MakeSpline({{2, -1}, {3, 0}});

  // Shared road after merge.
  auto const D = MakeSpline({{3, 0}, {4, 0}});

  std::vector<m2::SharedSpline> input{A, B_upper, B_lower, C_lower, C_upper, D};
  std::default_random_engine rnd{std::random_device{}()};

  for (size_t i = 0; i < 50; ++i)
  {
    // Shuffle helps to test for different starting seeds.
    std::shuffle(input.begin(), input.end(), rnd);

    auto const chains = OverlayProcessor::MergeSplines(input);

    // Always 2 chains: 4 features (5 pts) + 2 features (3 pts).
    TEST_EQUAL(chains.size(), 2, ());

    auto const & longer = (chains[0].size() >= chains[1].size()) ? chains[0] : chains[1];
    auto const & shorter = (chains[0].size() >= chains[1].size()) ? chains[1] : chains[0];
    TEST_EQUAL(longer.size(), 5, ());
    TEST_EQUAL(shorter.size(), 3, ());

    // The shorter chain is one carriageway: B_upper + C_upper or B_lower + C_lower.
    TEST(HasChain(chains, {{1, 0}, {2, 1}, {3, 0}}) || HasChain(chains, {{1, 0}, {2, -1}, {3, 0}}),
         ("Shorter chain should be one carriageway pair"));
  }
}

}  // namespace overlay_processor_test
