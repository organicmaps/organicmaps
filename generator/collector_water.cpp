#include "generator/collector_water.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/water_geometry.hpp"

#include "base/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/write_to_sink.hpp"

namespace generator
{
namespace
{
bool IsSourceLine(OsmElement const & element)
{
  auto const type = element.GetTag("waterway");
  return element.IsWay() && element.GetTag("area") != "yes" && element.GetTag("natural") != "water" &&
         (type == "river" || type == "canal" || type == "stream" || type == "ditch" || type == "drain" ||
          type == "fish_pass");
}
}  // namespace

std::shared_ptr<CollectorInterface> WaterGeometryCollector::Clone(IDRInterfacePtr const & cache) const
{
  return std::make_shared<WaterGeometryCollector>(GetFilename(), m_outputDir, m_affiliations, cache);
}

void WaterGeometryCollector::Collect(OsmElement const & element)
{
  if (!m_cache || !IsSourceLine(element) || element.m_nodes.size() < 2)
    return;
  // Display classification intentionally omits some waterways, e.g. culverts.
  // Preserve their source geometry so they cannot silently become land gaps.
  feature::FeatureBuilder feature;
  std::vector<m2::PointD> points;
  for (auto const id : element.m_nodes)
  {
    m2::PointD point;
    CHECK(m_cache->GetNode(id, point.y, point.x), (element.m_id, id, "Incomplete water source"));
    points.push_back(point);
  }
  feature.AssignPoints(std::move(points));
  feature.SetLinear();
  feature.SetOsmId(base::MakeOsmWay(element.m_id));
  feature.AddType(classif().GetTypeByPath({"waterway", "river"}));
  auto const layer = element.GetTag("layer");
  if (!layer.empty() && layer != "0")
    m_ids.insert(GetGeoObjectId(element).GetEncodedId());
  Store(feature, element);
}

std::string WaterGeometryCollector::TempFeatures(std::string const & country) const
{
  return GetTmpFilename() + "." + country + ".features";
}

WaterGeometryCollector::~WaterGeometryCollector()
{
  m_features.clear();
  for (auto const & country : m_countries)
    CHECK(Platform::RemoveFileIfExists(TempFeatures(country)), (country));
}

void WaterGeometryCollector::CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element)
{
  if (m_cache && IsSourceLine(element))
    return;
  Store(feature, element);
}

void WaterGeometryCollector::Store(feature::FeatureBuilder const & feature, OsmElement const & element)
{
  if (!(!feature.IsPoint() && (water::IsLine(feature.GetTypes()) || water::IsCoastline(feature.GetTypes()))) &&
      !(feature.IsArea() && water::IsArea(feature.GetTypes())))
    return;
  if (m_affiliations)
    for (auto const & country : m_affiliations->GetAffiliations(feature))
    {
      m_countries.insert(country);
      auto & collector = m_features[country];
      if (!collector)
        collector = std::make_unique<feature::FeaturesCollector>(TempFeatures(country));
      collector->Collect(feature);
    }
  auto const present = [](std::string_view value)
  { return !value.empty() && value != "no" && value != "false" && value != "0"; };
  if (present(element.GetTag("bridge")) || present(element.GetTag("tunnel")))
    m_ids.insert(GetGeoObjectId(element).GetEncodedId());
}

void WaterGeometryCollector::MergeInto(WaterGeometryCollector & collector) const
{
  CHECK(m_features.empty() && collector.m_features.empty(), ("Finish collectors before merging"));
  collector.m_ids.insert(m_ids.begin(), m_ids.end());
  for (auto const & country : m_countries)
  {
    collector.m_countries.insert(country);
    base::AppendFileToFile(TempFeatures(country), collector.TempFeatures(country));
  }
}

void WaterGeometryCollector::Finish()
{
  m_features.clear();
}

void WaterGeometryCollector::Save()
{
  FileWriter writer(GetFilename());
  for (auto const id : m_ids)
    WriteToSink(writer, id);
  CHECK(m_features.empty(), ("Finish collector before saving"));
  for (auto const & country : m_countries)
    CHECK(base::CopyFileX(TempFeatures(country), base::JoinPath(m_outputDir, country + ".water_features")), (country));
}
}  // namespace generator
