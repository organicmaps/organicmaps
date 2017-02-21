#pragma once

#include "search/categories_cache.hpp"
#include "search/cbv.hpp"
#include "search/mwm_context.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"

#include "std/map.hpp"
#include "std/shared_ptr.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

class FeatureType;

namespace search
{
namespace hotels_filter
{
struct Rating
{
  using Value = float;

  static Value const kDefault;

  static bool Lt(Value lhs, Value rhs) { return lhs + 0.05 < rhs; }
  static bool Gt(Value lhs, Value rhs) { return lhs > rhs + 0.05; }
  static bool Eq(Value lhs, Value rhs) { return !Lt(lhs, rhs) && !Gt(lhs, rhs); }

  template <typename Description>
  static Value Select(Description const & d)
  {
    return d.m_rating;
  }

  static char const * Name() { return "Rating"; }
};

struct PriceRate
{
  using Value = int;

  static Value const kDefault;

  static bool Lt(Value lhs, Value rhs) { return lhs < rhs; }
  static bool Gt(Value lhs, Value rhs) { return lhs > rhs; }
  static bool Eq(Value lhs, Value rhs) { return lhs == rhs; }

  template <typename Description>
  static Value Select(Description const & d)
  {
    return d.m_priceRate;
  }

  static char const * Name() { return "PriceRate"; }
};

struct Description
{
  void FromFeature(FeatureType & ft);

  Rating::Value m_rating = Rating::kDefault;
  PriceRate::Value m_priceRate = PriceRate::kDefault;
  unsigned m_types = 0;
};

struct Rule
{
  virtual ~Rule() = default;

  static bool IsIdentical(shared_ptr<Rule> const & lhs, shared_ptr<Rule> const & rhs);

  virtual bool Matches(Description const & d) const = 0;
  virtual bool IdenticalTo(Rule const & rhs) const = 0;
  virtual string ToString() const = 0;
};

string DebugPrint(Rule const & rule);

template <typename Field>
struct EqRule final : public Rule
{
  using Value = typename Field::Value;

  explicit EqRule(Value value) : m_value(value) {}

  // Rule overrides:
  bool Matches(Description const & d) const override
  {
    return Field::Eq(Field::Select(d), m_value);
  }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<EqRule const *>(&rhs);
    return r && Field::Eq(r->m_value, m_value);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[ " << Field::Name() << " == " << m_value << " ]";
    return os.str();
  }

  Value const m_value;
};

template <typename Field>
struct LtRule final : public Rule
{
  using Value = typename Field::Value;

  explicit LtRule(Value value) : m_value(value) {}

  // Rule overrides:
  bool Matches(Description const & d) const override
  {
    return Field::Lt(Field::Select(d), m_value);
  }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<LtRule const *>(&rhs);
    return r && Field::Eq(r->m_value, m_value);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[ " << Field::Name() << " < " << m_value << " ]";
    return os.str();
  }

  Value const m_value;
};

template <typename Field>
struct LeRule final : public Rule
{
  using Value = typename Field::Value;

  explicit LeRule(Value value) : m_value(value) {}

  // Rule overrides:
  bool Matches(Description const & d) const override
  {
    auto const value = Field::Select(d);
    return Field::Lt(value, m_value) || Field::Eq(value, m_value);
  }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<LeRule const *>(&rhs);
    return r && Field::Eq(r->m_value, m_value);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[ " << Field::Name() << " <= " << m_value << " ]";
    return os.str();
  }

  Value const m_value;
};

template <typename Field>
struct GtRule final : public Rule
{
  using Value = typename Field::Value;

  explicit GtRule(Value value) : m_value(value) {}

  // Rule overrides:
  bool Matches(Description const & d) const override
  {
    return Field::Gt(Field::Select(d), m_value);
  }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<GtRule const *>(&rhs);
    return r && Field::Eq(r->m_value, m_value);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[ " << Field::Name() << " > " << m_value << " ]";
    return os.str();
  }

  Value const m_value;
};

template <typename Field>
struct GeRule final : public Rule
{
  using Value = typename Field::Value;

  explicit GeRule(Value value) : m_value(value) {}

  // Rule overrides:
  bool Matches(Description const & d) const override
  {
    auto const value = Field::Select(d);
    return Field::Gt(value, m_value) || Field::Eq(value, m_value);
  }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<GeRule const *>(&rhs);
    return r && Field::Eq(r->m_value, m_value);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[ " << Field::Name() << " >= " << m_value << " ]";
    return os.str();
  }

  Value const m_value;
};

struct AndRule final : public Rule
{
  AndRule(shared_ptr<Rule> lhs, shared_ptr<Rule> rhs) : m_lhs(move(lhs)), m_rhs(move(rhs)) {}

  // Rule overrides:
  bool Matches(Description const & d) const override
  {
    bool matches = true;
    if (m_lhs)
      matches = matches && m_lhs->Matches(d);
    if (m_rhs)
      matches = matches && m_rhs->Matches(d);
    return matches;
  }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<AndRule const *>(&rhs);
    return r && IsIdentical(m_lhs, r->m_lhs) && IsIdentical(m_rhs, r->m_rhs);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[";
    os << (m_lhs ? m_lhs->ToString() : "<none>");
    os << " && ";
    os << (m_rhs ? m_rhs->ToString() : "<none>");
    os << "]";
    return os.str();
  }

  shared_ptr<Rule> m_lhs;
  shared_ptr<Rule> m_rhs;
};

struct OrRule final : public Rule
{
  OrRule(shared_ptr<Rule> lhs, shared_ptr<Rule> rhs) : m_lhs(move(lhs)), m_rhs(move(rhs)) {}

  // Rule overrides:
  bool Matches(Description const & d) const override
  {
    bool matches = false;
    if (m_lhs)
      matches = matches || m_lhs->Matches(d);
    if (m_rhs)
      matches = matches || m_rhs->Matches(d);
    return matches;
  }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<OrRule const *>(&rhs);
    return r && IsIdentical(m_lhs, r->m_lhs) && IsIdentical(m_rhs, r->m_rhs);
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[";
    os << (m_lhs ? m_lhs->ToString() : "<none>");
    os << " || ";
    os << (m_rhs ? m_rhs->ToString() : "<none>");
    os << "]";
    return os.str();
  }

  shared_ptr<Rule> m_lhs;
  shared_ptr<Rule> m_rhs;
};

struct OneOfRule final : public Rule
{
  explicit OneOfRule(unsigned types) : m_types(types) {}

  // Rule overrides:
  bool Matches(Description const & d) const override { return (d.m_types & m_types) != 0; }

  bool IdenticalTo(Rule const & rhs) const override
  {
    auto const * r = dynamic_cast<OneOfRule const *>(&rhs);
    return r && m_types == r->m_types;
  }

  string ToString() const override
  {
    ostringstream os;
    os << "[ one of:";
    for (size_t i = 0; i < static_cast<size_t>(ftypes::IsHotelChecker::Type::Count); ++i)
    {
      unsigned const bit = 1U << i;
      if ((m_types & bit) == 0)
        continue;

      auto const type = static_cast<ftypes::IsHotelChecker::Type>(i);
      os << " " << ftypes::IsHotelChecker::GetHotelTypeTag(type);
    }
    os << " ]";
    return os.str();
  }

  unsigned m_types;
};

template <typename Field>
shared_ptr<Rule> Eq(typename Field::Value value)
{
  return make_shared<EqRule<Field>>(value);
}

template <typename Field>
shared_ptr<Rule> Lt(typename Field::Value value)
{
  return make_shared<LtRule<Field>>(value);
}

template <typename Field>
shared_ptr<Rule> Le(typename Field::Value value)
{
  return make_shared<LeRule<Field>>(value);
}

template <typename Field>
inline shared_ptr<Rule> Gt(typename Field::Value value)
{
  return make_shared<GtRule<Field>>(value);
}

template <typename Field>
shared_ptr<Rule> Ge(typename Field::Value value)
{
  return make_shared<GeRule<Field>>(value);
}

inline shared_ptr<Rule> And(shared_ptr<Rule> lhs, shared_ptr<Rule> rhs)
{
  return make_shared<AndRule>(lhs, rhs);
}

inline shared_ptr<Rule> Or(shared_ptr<Rule> lhs, shared_ptr<Rule> rhs)
{
  return make_shared<OrRule>(lhs, rhs);
}

inline shared_ptr<Rule> Is(ftypes::IsHotelChecker::Type type)
{
  CHECK(type != ftypes::IsHotelChecker::Type::Count, ());
  return make_shared<OneOfRule>(1U << static_cast<unsigned>(type));
}

inline shared_ptr<Rule> OneOf(unsigned types) { return make_shared<OneOfRule>(types); }

class HotelsFilter
{
public:
  using Descriptions = vector<pair<uint32_t, Description>>;

  class ScopedFilter
  {
  public:
    ScopedFilter(MwmSet::MwmId const & mwmId, Descriptions const & descriptions,
                 shared_ptr<Rule> rule);

    bool Matches(FeatureID const & fid) const;

  private:
    MwmSet::MwmId const m_mwmId;
    Descriptions const & m_descriptions;
    shared_ptr<Rule> const m_rule;
  };

  HotelsFilter(HotelsCache & hotels);

  unique_ptr<ScopedFilter> MakeScopedFilter(MwmContext const & context, shared_ptr<Rule> rule);

  void ClearCaches();

private:
  Descriptions const & GetDescriptions(MwmContext const & context);

  HotelsCache & m_hotels;
  map<MwmSet::MwmId, Descriptions> m_descriptions;
};
}  // namespace hotels_filter
}  // namespace search
