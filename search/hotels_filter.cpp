#include "search/hotels_filter.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>

using namespace std;

namespace search
{
namespace hotels_filter
{
// static
typename Rating::Value const Rating::kDefault = 0;

// static
typename PriceRate::Value const PriceRate::kDefault = 0;

// Description -------------------------------------------------------------------------------------
void Description::FromFeature(FeatureType & ft)
{
  m_rating = Rating::kDefault;
  m_priceRate = PriceRate::kDefault;

  string const rating = ft.GetMetadata(feature::Metadata::FMD_RATING);
  if (!rating.empty())
  {
    float r;
    if (strings::to_float(rating, r))
      m_rating = r;
  }

  string const priceRate = ft.GetMetadata(feature::Metadata::FMD_PRICE_RATE);
  if (!priceRate.empty())
  {
    int pr;
    if (strings::to_int(priceRate, pr))
      m_priceRate = pr;
  }

  m_types = ftypes::IsHotelChecker::Instance().GetHotelTypesMask(ft);
}

// Rule --------------------------------------------------------------------------------------------
// static
bool Rule::IsIdentical(shared_ptr<Rule> const & lhs, shared_ptr<Rule> const & rhs)
{
  if (lhs && !rhs)
    return false;
  if (!lhs && rhs)
    return false;

  if (lhs && rhs && !lhs->IdenticalTo(*rhs))
    return false;

  return true;
}

string DebugPrint(Rule const & rule) { return rule.ToString(); }

// HotelsFilter::ScopedFilter ----------------------------------------------------------------------
HotelsFilter::ScopedFilter::ScopedFilter(MwmSet::MwmId const & mwmId,
                                         Descriptions const & descriptions, shared_ptr<Rule> rule)
  : m_mwmId(mwmId), m_descriptions(descriptions), m_rule(rule)
{
  CHECK(m_rule.get(), ());
}

bool HotelsFilter::ScopedFilter::Matches(FeatureID const & fid) const
{
  if (fid.m_mwmId != m_mwmId)
    return false;

  auto it =
      lower_bound(m_descriptions.begin(), m_descriptions.end(),
                  make_pair(fid.m_index, Description{}),
                  [](pair<uint32_t, Description> const & lhs,
                     pair<uint32_t, Description> const & rhs) { return lhs.first < rhs.first; });
  if (it == m_descriptions.end() || it->first != fid.m_index)
    return false;

  return m_rule->Matches(it->second);
}

// HotelsFilter ------------------------------------------------------------------------------------
HotelsFilter::HotelsFilter(HotelsCache & hotels): m_hotels(hotels) {}

unique_ptr<HotelsFilter::ScopedFilter> HotelsFilter::MakeScopedFilter(MwmContext const & context,
                                                                      shared_ptr<Rule> rule)
{
  if (!rule)
    return {};
  return make_unique<ScopedFilter>(context.GetId(), GetDescriptions(context), rule);
}

void HotelsFilter::ClearCaches()
{
  m_descriptions.clear();
}

HotelsFilter::Descriptions const & HotelsFilter::GetDescriptions(MwmContext const & context)
{
  auto const & mwmId = context.GetId();
  auto const it = m_descriptions.find(mwmId);
  if (it != m_descriptions.end())
    return it->second;

  auto const hotels = m_hotels.Get(context);
  auto & descriptions = m_descriptions[mwmId];
  hotels.ForEach([&descriptions, &context](uint64_t bit) {
    auto const id = base::asserted_cast<uint32_t>(bit);

    Description description;
    if (auto ft = context.GetFeature(id))
      description.FromFeature(*ft);
    descriptions.emplace_back(id, description);
  });
  return descriptions;
}
}  // namespace hotels_filter
}  // namespace search
