#include "generator/collector_city_area.hpp"

#include "generator/feature_generator.hpp"
#include "generator/final_processor_utils.hpp"
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
    m_writer(std::make_unique<FeatureBuilderWriter<MaxAccuracy>>(GetTmpFilename())) {}

std::shared_ptr<CollectorInterface> CityAreaCollector::Clone(
    std::shared_ptr<cache::IntermediateDataReaderInterface> const &) const
{
  return std::make_shared<CityAreaCollector>(GetFilename());
}

void CityAreaCollector::CollectFeature(FeatureBuilder const & feature, OsmElement const &)
{
  auto const & isCityTownOrVillage = ftypes::IsCityTownOrVillageChecker::Instance();
  if (!(feature.IsArea() && isCityTownOrVillage(feature.GetTypes())))
    return;

  auto copy = feature;
  if (copy.PreSerialize())
    m_writer->Write(copy);
}

void CityAreaCollector::Finish()
{
  m_writer.reset({});
}

void CityAreaCollector::Save()
{
  CHECK(!m_writer, ("Finish() has not been called."));
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void CityAreaCollector::OrderCollectedData()
{
  auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(GetFilename());
  Order(fbs);
  FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(GetFilename());
  for (auto const & fb : fbs)
    writer.Write(fb);
}

void CityAreaCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CityAreaCollector::MergeInto(CityAreaCollector & collector) const
{
  CHECK(!m_writer || !collector.m_writer, ("Finish() has not been called."));
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}
}  // namespace generator
