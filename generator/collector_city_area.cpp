#include "generator/collector_city_area.hpp"

#include "generator/feature_generator.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <iterator>

using namespace feature;
using namespace feature::serialization_policy;

namespace generator
{
CityAreaCollector::CityAreaCollector(std::string const & filename)
  : CollectorInterface(filename),
    m_witer(std::make_unique<FeatureBuilderWriter<MaxAccuracy>>(GetTmpFilename())) {}

std::shared_ptr<CollectorInterface>
CityAreaCollector::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CityAreaCollector>(GetFilename());
}

void CityAreaCollector::CollectFeature(FeatureBuilder const & feature, OsmElement const &)
{
  if (!(feature.IsArea() && ftypes::IsCityTownOrVillage(feature.GetTypes())))
    return;

  auto copy = feature;
  if (copy.PreSerialize())
    m_witer->Write(copy);
}

void CityAreaCollector::Finish()
{
  m_witer.reset({});
}

void CityAreaCollector::Save()
{
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void CityAreaCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CityAreaCollector::MergeInto(CityAreaCollector & collector) const
{
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}
}  // namespace generator
