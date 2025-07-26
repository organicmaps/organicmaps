#include "indexer/drules_selector.hpp"
#include "indexer/drules_selector_parser.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace drule
{
using namespace std;

namespace
{

class CompositeSelector : public ISelector
{
public:
  explicit CompositeSelector(size_t capacity) { m_selectors.reserve(capacity); }

  void Add(unique_ptr<ISelector> && selector) { m_selectors.emplace_back(std::move(selector)); }

  // ISelector overrides:
  bool Test(FeatureType & ft, int zoom) const override
  {
    for (auto const & selector : m_selectors)
      if (!selector->Test(ft, zoom))
        return false;
    return true;
  }

private:
  vector<unique_ptr<ISelector>> m_selectors;
};

// Runtime feature style selector implementation
template <typename TType>
class Selector : public ISelector
{
public:
  // Signature of function which takes a property from a feature
  typedef bool (*TGetFeatureTagValueFn)(FeatureType &, int, TType & value);

  Selector(TGetFeatureTagValueFn fn, SelectorOperatorType op, TType const & value)
    : m_getFeatureValueFn(fn)
    , m_evalFn(nullptr)
    , m_value(value)
  {
    ASSERT(fn != nullptr, ());

    switch (op)
    {
    case SelectorOperatorUnknown: m_evalFn = nullptr; break;
    case SelectorOperatorNotEqual: m_evalFn = &Selector<TType>::NotEqual; break;
    case SelectorOperatorLessOrEqual: m_evalFn = &Selector<TType>::LessOrEqual; break;
    case SelectorOperatorGreaterOrEqual: m_evalFn = &Selector<TType>::GreaterOrEqual; break;
    case SelectorOperatorEqual: m_evalFn = &Selector<TType>::Equal; break;
    case SelectorOperatorLess: m_evalFn = &Selector<TType>::Less; break;
    case SelectorOperatorGreater: m_evalFn = &Selector<TType>::Greater; break;
    case SelectorOperatorIsNotSet: m_evalFn = &Selector<TType>::IsNotSet; break;
    case SelectorOperatorIsSet: m_evalFn = &Selector<TType>::IsSet; break;
    }

    ASSERT(m_evalFn != nullptr, ("Unknown or unexpected selector operator type"));
    if (nullptr == m_evalFn)
      m_evalFn = &Selector<TType>::Unknown;
  }

  // ISelector overrides:
  bool Test(FeatureType & ft, int zoom) const override
  {
    TType tagValue;
    if (!m_getFeatureValueFn(ft, zoom, tagValue))
      return false;
    return (this->*m_evalFn)(tagValue);
  }

private:
  bool Unknown(TType const &) const { return false; }
  bool NotEqual(TType const & tagValue) const { return tagValue != m_value; }
  bool LessOrEqual(TType const & tagValue) const { return tagValue <= m_value; }
  bool GreaterOrEqual(TType const & tagValue) const { return tagValue >= m_value; }
  bool Equal(TType const & tagValue) const { return tagValue == m_value; }
  bool Less(TType const & tagValue) const { return tagValue < m_value; }
  bool Greater(TType const & tagValue) const { return tagValue > m_value; }
  bool IsNotSet(TType const & tagValue) const { return tagValue == TType(); }
  bool IsSet(TType const & tagValue) const { return tagValue != TType(); }

  typedef bool (Selector<TType>::*TOperationFn)(TType const &) const;

  TGetFeatureTagValueFn m_getFeatureValueFn;
  TOperationFn m_evalFn;
  TType const m_value;
};

class HasSelector : public ISelector
{
public:
  typedef bool (*THasFeatureTagValueFn)(FeatureType &, int);

  HasSelector(THasFeatureTagValueFn fn, SelectorOperatorType op)
    : m_hasFeatureValueFn(fn)
    , m_testHas(op == SelectorOperatorIsSet)
  {
    ASSERT(op == SelectorOperatorIsSet || op == SelectorOperatorIsNotSet, ());
    ASSERT(fn != nullptr, ());
  }

  // ISelector overrides:
  bool Test(FeatureType & ft, int zoom) const override { return m_hasFeatureValueFn(ft, zoom) == m_testHas; }

private:
  THasFeatureTagValueFn m_hasFeatureValueFn;
  bool m_testHas;
};

/*
uint32_t TagSelectorToType(string value)
{
  vector<string> path;
  strings::ParseCSVRow(value, '=', path);
  return path.size() > 0 && path.size() <= 2 ? classif().GetTypeByPathSafe(path) : 0;
}

class TypeSelector : public ISelector
{
public:
  TypeSelector(uint32_t type, SelectorOperatorType op) : m_type(type)
  {
    m_equals = op == SelectorOperatorEqual;
  }

  bool Test(FeatureType & ft) const override
  {
    bool found = false;
    ft.ForEachType([&found, this](uint32_t type)
    {
      ftype::TruncValue(type, ftype::GetLevel(m_type));
      if (type == m_type)
        found = true;
    });
    return found == m_equals;
  }

private:
  uint32_t m_type;
  bool m_equals;
};
*/

// Feature tag value evaluator for tag 'population'
bool GetPopulation(FeatureType & ft, int, uint64_t & population)
{
  population = ftypes::GetPopulation(ft);
  return true;
}

bool HasName(FeatureType & ft, int)
{
  return ft.HasName();
}

// Feature tag value evaluator for tag 'bbox_area' (bounding box area in sq.meters)
bool GetBoundingBoxArea(FeatureType & ft, int zoom, double & sqM)
{
  if (feature::GeomType::Area != ft.GetGeomType())
    return false;

  // https://github.com/organicmaps/organicmaps/issues/2840
  sqM = mercator::AreaOnEarth(ft.GetLimitRect(zoom));
  return true;
}
}  // namespace

unique_ptr<ISelector> ParseSelector(string const & str)
{
  SelectorExpression e;
  if (!ParseSelector(str, e))
  {
    // bad string format
    LOG(LDEBUG, ("Invalid selector format:", str));
    return {};
  }

  if (e.m_tag == "population")
  {
    uint64_t value = 0;
    if (!e.m_value.empty() && !strings::to_uint64(e.m_value, value))
    {
      // bad string format
      LOG(LDEBUG, ("Invalid selector:", str));
      return {};
    }
    return make_unique<Selector<uint64_t>>(&GetPopulation, e.m_operator, value);
  }
  else if (e.m_tag == "name")
  {
    return make_unique<HasSelector>(&HasName, e.m_operator);
  }
  else if (e.m_tag == "bbox_area")
  {
    double value = 0;
    if (!e.m_value.empty() && (!strings::to_double(e.m_value, value) || value < 0))
    {
      // bad string format
      LOG(LDEBUG, ("Invalid selector:", str));
      return {};
    }
    return make_unique<Selector<double>>(&GetBoundingBoxArea, e.m_operator, value);
  }
  //  else if (e.m_tag == "extra_tag")
  //  {
  //    ASSERT(false, ());
  //    uint32_t const type = TagSelectorToType(e.m_value);
  //    if (type == Classificator::INVALID_TYPE)
  //    {
  //      // Type was not found.
  //      LOG(LDEBUG, ("Invalid selector:", str));
  //      return unique_ptr<ISelector>();
  //    }
  //    return make_unique<TypeSelector>(type, e.m_operator);
  //  }

  LOG(LERROR, ("Unrecognized selector:", str));
  return {};
}

unique_ptr<ISelector> ParseSelector(vector<string> const & strs)
{
  unique_ptr<CompositeSelector> cs = make_unique<CompositeSelector>(strs.size());

  for (string const & str : strs)
  {
    unique_ptr<ISelector> s = ParseSelector(str);
    if (nullptr == s)
    {
      LOG(LDEBUG, ("Invalid composite selector:", str));
      return unique_ptr<ISelector>();
    }
    cs->Add(std::move(s));
  }

  return unique_ptr<ISelector>(cs.release());
}

}  // namespace drule
