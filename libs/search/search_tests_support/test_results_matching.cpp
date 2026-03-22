#include "search/search_tests_support/test_results_matching.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"

#include <algorithm>
#include <sstream>

namespace search
{
namespace tests_support
{
using generator::tests_support::TestFeature;

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

std::string ExactMatchingRule::ToString() const
{
  std::ostringstream os;
  os << "ExactMatchingRule [ " << DebugPrint(m_mwmId) << ", " << DebugPrint(m_feature) << " ]";
  return os.str();
}

AlternativesMatchingRule::AlternativesMatchingRule(std::vector<std::shared_ptr<MatchingRule>> && rules)
  : m_rules(std::move(rules))
{}

bool AlternativesMatchingRule::Matches(FeatureType & feature) const
{
  for (auto const & rule : m_rules)
    if (rule->Matches(feature))
      return true;
  return false;
}

std::string AlternativesMatchingRule::ToString() const
{
  std::ostringstream os;
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

bool MatchResults(DataSource const & dataSource, std::vector<std::shared_ptr<MatchingRule>> rules,
                  std::vector<search::Result> const & actual)
{
  std::vector<FeatureID> resultIds;
  resultIds.reserve(actual.size());
  for (auto const & a : actual)
    resultIds.push_back(a.GetFeatureID());
  std::sort(resultIds.begin(), resultIds.end());

  std::vector<std::string> unexpected;
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

  std::ostringstream os;
  os << "Unsatisfied rules:" << std::endl;
  for (auto const & e : rules)
    os << "  " << DebugPrint(*e) << std::endl;
  os << "Unexpected retrieved features:" << std::endl;
  for (auto const & u : unexpected)
    os << "  " << u << std::endl;

  LOG(LWARNING, (os.str()));
  return false;
}

bool MatchResults(DataSource const & dataSource, std::vector<std::shared_ptr<MatchingRule>> rules,
                  search::Results const & actual)
{
  std::vector<search::Result> const results(actual.begin(), actual.end());
  return MatchResults(dataSource, rules, results);
}

bool ResultMatches(DataSource const & dataSource, std::shared_ptr<MatchingRule> rule, search::Result const & result)
{
  bool matches = false;
  dataSource.ReadFeature([&](FeatureType & ft) { matches = rule->Matches(ft); }, result.GetFeatureID());
  return matches;
}

bool AlternativeMatch(DataSource const & dataSource, std::vector<std::vector<std::shared_ptr<MatchingRule>>> rulesList,
                      std::vector<search::Result> const & results)
{
  return std::any_of(rulesList.begin(), rulesList.end(), [&](std::vector<std::shared_ptr<MatchingRule>> const & rules)
  { return MatchResults(dataSource, rules, results); });
}

std::string DebugPrint(MatchingRule const & rule)
{
  return rule.ToString();
}
}  // namespace tests_support
}  // namespace search
