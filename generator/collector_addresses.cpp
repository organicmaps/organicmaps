#include "generator/collector_addresses.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"

using namespace feature;

namespace generator
{
CollectorAddresses::CollectorAddresses(std::string const & filename)
  : CollectorInterface(filename)
{
  m_writer.exceptions(std::fstream::failbit | std::fstream::badbit);
  m_writer.open(GetTmpFilename());
}

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
    m_writer << addr << "\n";
}

void CollectorAddresses::Finish()
{
  if (m_writer.is_open())
    m_writer.close();
}

void CollectorAddresses::Save()
{
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void CollectorAddresses::Merge(CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CollectorAddresses::MergeInto(CollectorAddresses & collector) const
{
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}
}  // namespace generator
