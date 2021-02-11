#include "generator/collector_boundary_postcode.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <iterator>

using namespace feature;

namespace generator
{
BoundaryPostcodeCollector::BoundaryPostcodeCollector(
    std::string const & filename, std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache)
  : CollectorInterface(filename)
  , m_writer(std::make_unique<FileWriter>(GetTmpFilename()))
  , m_cache(cache)
  , m_featureMakerSimple(cache)
{
}

std::shared_ptr<CollectorInterface> BoundaryPostcodeCollector::Clone(
    std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache) const
{
  return std::make_shared<BoundaryPostcodeCollector>(GetFilename(), cache ? cache : m_cache);
}

void BoundaryPostcodeCollector::Collect(OsmElement const & el)
{
  if (el.m_type != OsmElement::EntityType::Relation)
    return;

  if (!el.HasTag("boundary", "postal_code"))
    return;

  auto const postcode = el.GetTag("postal_code");
  if (postcode.empty())
    return;

  auto osmElementCopy = el;
  feature::FeatureBuilder feature;
  m_featureMakerSimple.Add(osmElementCopy);

  while (m_featureMakerSimple.GetNextFeature(feature))
  {
    utils::WriteString(*m_writer, postcode);
    rw::WriteVectorOfPOD(*m_writer, feature.GetOuterGeometry());
  }
}

void BoundaryPostcodeCollector::Finish() { m_writer.reset(); }

void BoundaryPostcodeCollector::Save()
{
  CHECK(!m_writer, ("Finish() has not been called."));
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void BoundaryPostcodeCollector::OrderCollectedData()
{
  std::vector<std::pair<std::string, FeatureBuilder::PointSeq>> collectedData;
  {
    FileReader reader(GetFilename());
    ReaderSource src(reader);
    while (src.Size() > 0)
    {
      collectedData.push_back({});
      utils::ReadString(src, collectedData.back().first);
      rw::ReadVectorOfPOD(src, collectedData.back().second);
    }
  }
  std::sort(std::begin(collectedData), std::end(collectedData));
  FileWriter writer(GetFilename());
  for (auto const & p : collectedData)
  {
    utils::WriteString(writer, p.first);
    rw::WriteVectorOfPOD(writer, p.second);
  }
}

void BoundaryPostcodeCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void BoundaryPostcodeCollector::MergeInto(BoundaryPostcodeCollector & collector) const
{
  CHECK(!m_writer || !collector.m_writer, ("Finish() has not been called."));
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}
}  // namespace generator
