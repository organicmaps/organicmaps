#include "generator/collector_addresses.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/ftypes_matcher.hpp"

using namespace feature;

#include <fstream>

namespace generator
{
CollectorAddresses::CollectorAddresses(std::string const & filename)
  : CollectorInterface(filename) {}

std::shared_ptr<CollectorInterface>
CollectorAddresses::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CollectorAddresses>(GetFilename());
}

void CollectorAddresses::CollectFeature(feature::FeatureBuilder const & feature, OsmElement const &)
{
  std::string addr;
  auto const & checker = ftypes::IsBuildingChecker::Instance();
  if (checker(feature.GetTypes()) && feature.FormatFullAddress(addr))
    m_stringStream << addr << "\n";
}

void CollectorAddresses::Save()
{
  std::ofstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(GetFilename());
  stream << m_stringStream.str();
}

void CollectorAddresses::Merge(CollectorInterface const * collector)
{
  CHECK(collector, ());

  collector->MergeInto(const_cast<CollectorAddresses *>(this));
}

void CollectorAddresses::MergeInto(CollectorAddresses * collector) const
{
  CHECK(collector, ());

  collector->m_stringStream << m_stringStream.str();
}
}  // namespace generator
