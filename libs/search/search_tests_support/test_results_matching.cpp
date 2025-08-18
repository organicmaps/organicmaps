#include "search/search_tests_support/test_results_matching.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"

#include <algorithm>
#include <sstream>

using namespace std;
using namespace generator::tests_support;

namespace search
{
namespace tests_support
{
ExactMatchingRule::ExactMatchingRule(MwmSet::MwmId const & mwmId, TestFeature const & feature)
  : m_mwmId(mwmId)
  , m_feature(feature)
{}

bool ExactMatchingRule::Matches(FeatureType & feature) const
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

AlternativesMatchingRule::AlternativesMatchingRule(vector<shared_ptr<MatchingRule>> && rules)
  : m_rules(std::move(rules))
{}

bool AlternativesMatchingRule::Matches(FeatureType & feature) const
{
  for (auto const & rule : m_rules)
    if (rule->Matches(feature))
      return true;
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

bool MatchResults(DataSource const & dataSource, vector<shared_ptr<MatchingRule>> rules,
                  vector<search::Result> const & actual)
{
  vector<FeatureID> resultIds;
  resultIds.reserve(actual.size());
  for (auto const & a : actual)
    resultIds.push_back(a.GetFeatureID());
  sort(resultIds.begin(), resultIds.end());

  vector<string> unexpected;
  auto removeMatched = [&rules, &unexpected](FeatureType & feature)
  {
    for (auto it = rules.begin(); it != rules.end(); ++it)
    {
      if ((*it)->Matches(feature))
      {
        rules.erase(it);
        return;
      }
    }
    unexpected.push_back(feature.DebugString());
  };
  dataSource.ReadFeatures(removeMatched, resultIds);

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

bool MatchResults(DataSource const & dataSource, vector<shared_ptr<MatchingRule>> rules, search::Results const & actual)
{
  vector<search::Result> const results(actual.begin(), actual.end());
  return MatchResults(dataSource, rules, results);
}

bool ResultMatches(DataSource const & dataSource, shared_ptr<MatchingRule> rule, search::Result const & result)
{
  bool matches = false;
  dataSource.ReadFeature([&](FeatureType & ft) { matches = rule->Matches(ft); }, result.GetFeatureID());
  return matches;
}

bool AlternativeMatch(DataSource const & dataSource, vector<vector<shared_ptr<MatchingRule>>> rulesList,
                      std::vector<search::Result> const & results)
{
  return any_of(rulesList.begin(), rulesList.end(), [&](vector<shared_ptr<MatchingRule>> const & rules)
  { return MatchResults(dataSource, rules, results); });
}

string DebugPrint(MatchingRule const & rule)
{
  return rule.ToString();
}
}  // namespace tests_support
}  // namespace search
