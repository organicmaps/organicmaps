#include "generator/collector_addresses.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/ftypes_matcher.hpp"

using namespace feature;

namespace generator
{
CollectorAddresses::CollectorAddresses(std::string const & filename)
  : m_addrWriter(std::make_unique<FileWriter>(filename)) {}

void CollectorAddresses::CollectFeature(FeatureBuilder const & feature, OsmElement const &)
{
  std::string addr;
  auto const & checker = ftypes::IsBuildingChecker::Instance();
  if (checker(feature.GetTypes()) && feature.FormatFullAddress(addr))
    m_addrWriter->Write(addr.c_str(), addr.size());
}
}  // namespace generator
