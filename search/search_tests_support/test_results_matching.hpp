#pragma once

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
class TestFeature;

class MatchingRule
{
public:
  virtual ~MatchingRule() = default;

  virtual bool Matches(FeatureType const & feature) const = 0;
  virtual string ToString() const = 0;
};

class ExactMatchingRule : public MatchingRule
{
public:
  ExactMatchingRule(MwmSet::MwmId const & mwmId, TestFeature & feature);

  // MatchingRule overrides:
  bool Matches(FeatureType const & feature) const override;
  string ToString() const override;

private:
  MwmSet::MwmId m_mwmId;
  TestFeature & m_feature;
};

class AlternativesMatchingRule : public MatchingRule
{
public:
  AlternativesMatchingRule(initializer_list<shared_ptr<MatchingRule>> rules);

  // MatchingRule overrides:
  bool Matches(FeatureType const & feature) const override;
  string ToString() const override;

private:
  vector<shared_ptr<MatchingRule>> m_rules;
};

template <typename... TArgs>
shared_ptr<MatchingRule> ExactMatch(TArgs &&... args)
{
  return make_shared<ExactMatchingRule>(forward<TArgs>(args)...);
}

template <typename... TArgs>
shared_ptr<MatchingRule> AlternativesMatch(TArgs &&... args)
{
  return make_shared<AlternativesMatchingRule>(forward<TArgs>(args)...);
}

bool MatchResults(Index const & index, vector<shared_ptr<MatchingRule>> rules,
                  vector<search::Result> const & actual);

string DebugPrint(MatchingRule const & rule);
}  // namespace tests_support
}  // namespace search
