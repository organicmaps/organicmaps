#pragma once

#include "search/search_tests_support/test_feature.hpp"

#include "search/result.hpp"

#include "indexer/mwm_set.hpp"

#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

class FeatureType;
class Index;

namespace search
{
namespace tests_support
{
class MatchingRule
{
public:
  virtual ~MatchingRule() = default;

  virtual bool Matches(FeatureType const & feature) const = 0;
  virtual string ToString() const = 0;
};

class ExactMatch : public MatchingRule
{
public:
  ExactMatch(MwmSet::MwmId const & mwmId, shared_ptr<TestFeature> feature);

  // MatchingRule overrides:
  bool Matches(FeatureType const & feature) const override;
  string ToString() const override;

private:
  MwmSet::MwmId m_mwmId;
  shared_ptr<TestFeature> m_feature;
};

class AlternativesMatch : public MatchingRule
{
public:
  AlternativesMatch(initializer_list<shared_ptr<MatchingRule>> rules);

  // MatchingRule overrides:
  bool Matches(FeatureType const & feature) const override;
  string ToString() const override;

private:
  vector<shared_ptr<MatchingRule>> m_rules;
};

bool MatchResults(Index const & index, vector<shared_ptr<MatchingRule>> rules,
                  vector<search::Result> const & actual);

string DebugPrint(MatchingRule const & rule);
}  // namespace tests_support
}  // namespace search
