#include "search/search_tests_support/test_results_matching.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"

#include "std/sstream.hpp"

using namespace generator::tests_support;

namespace search
{
namespace tests_support
{
ExactMatchingRule::ExactMatchingRule(MwmSet::MwmId const & mwmId, TestFeature const & feature)
  : m_mwmId(mwmId), m_feature(feature)
{
}

bool ExactMatchingRule::Matches(FeatureType const & feature) const
{
  if (m_mwmId != feature.GetID().m_mwmId)
    return false;
  return m_feature.Matches(feature);
}

string ExactMatchingRule::ToString() const
{
  ostringstream os;
  os << "ExactMatchingRule [ " << DebugPrint(m_mwmId) << ", " << DebugPrint(m_feature) << " ]";
  return os.str();
}

AlternativesMatchingRule::AlternativesMatchingRule(initializer_list<shared_ptr<MatchingRule>> rules)
  : m_rules(move(rules))
{
}

bool AlternativesMatchingRule::Matches(FeatureType const & feature) const
{
  for (auto const & rule : m_rules)
  {
    if (rule->Matches(feature))
      return true;
  }
  return false;
}

string AlternativesMatchingRule::ToString() const
{
  ostringstream os;
  os << "OrRule [ ";
  for (auto it = m_rules.cbegin(); it != m_rules.cend(); ++it)
  {
    os << (*it)->ToString();
    if (it + 1 != m_rules.cend())
      os << " | ";
  }
  os << " ]";
  return os.str();
}

bool MatchResults(Index const & index, vector<shared_ptr<MatchingRule>> rules,
                  vector<search::Result> const & actual)
{
  vector<FeatureID> resultIds;
  for (auto const & a : actual)
    resultIds.push_back(a.GetFeatureID());
  sort(resultIds.begin(), resultIds.end());

  vector<string> unexpected;
  auto removeMatched = [&rules, &unexpected](FeatureType const & feature)
  {
    for (auto it = rules.begin(); it != rules.end(); ++it)
    {
      if ((*it)->Matches(feature))
      {
        rules.erase(it);
        return;
      }
    }
    unexpected.push_back(DebugPrint(feature) + " from " + DebugPrint(feature.GetID().m_mwmId));
  };
  index.ReadFeatures(removeMatched, resultIds);

  if (rules.empty() && unexpected.empty())
    return true;

  ostringstream os;
  os << "Unsatisfied rules:" << endl;
  for (auto const & e : rules)
    os << "  " << DebugPrint(*e) << endl;
  os << "Unexpected retrieved features:" << endl;
  for (auto const & u : unexpected)
    os << "  " << u << endl;

  LOG(LWARNING, (os.str()));
  return false;
}

bool MatchResults(Index const & index, vector<shared_ptr<MatchingRule>> rules,
                  search::Results const & actual)
{
  vector<search::Result> const results(actual.begin(), actual.end());
  return MatchResults(index, rules, results);
}

bool ResultMatches(Index const & index, shared_ptr<MatchingRule> rule,
                   search::Result const & result)
{
  bool matches = false;
  index.ReadFeature([&](FeatureType & ft) { matches = rule->Matches(ft); }, result.GetFeatureID());
  return matches;
}

string DebugPrint(MatchingRule const & rule) { return rule.ToString(); }
}  // namespace tests_support
}  // namespace search
