#include "generator/water_geometry_builder.hpp"

#include "generator/borders.hpp"
#include "generator/collector_water.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/water_geometry.hpp"

#include "coding/file_reader.hpp"
#include "coding/files_container.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <fstream>
#include <set>

namespace generator
{
void ResetWaterGeometry(std::string const & temporaryDirectory)
{
  for (auto const * extension : {".water_topology", ".water_features"})
  {
    Platform::FilesList files;
    Platform::GetFilesByExt(temporaryDirectory, extension, files);
    for (auto const & file : files)
      CHECK(Platform::RemoveFileIfExists(base::JoinPath(temporaryDirectory, file)), (file));
  }
}

bool BuildWaterGeometrySection(feature::GenerateInfo const & info, std::string const & name,
                               m2::RectD const & sourceBounds)
{
  if (!Platform::IsFileExistsByFullPath(info.GetTmpFileName(name, ".water_topology")))
  {
    LOG(LERROR, ("Regenerate intermediate features with --generate_water_routing before building the section"));
    return false;
  }
  std::string marker;
  std::ifstream(info.GetTmpFileName(name, ".water_topology")) >> marker;
  if (marker != "1c" && marker != "1n")
    return false;
  bool const coastsIncluded = marker == "1c";
  std::vector<m2::RegionD> borders;
  if (!borders::LoadBorders(base::JoinPath(info.m_targetDir, BORDERS_DIR, name + BORDERS_EXTENSION), borders))
    return false;
  std::vector<std::vector<m2::PointD>> coverage;
  for (auto const & border : borders)
  {
    // A partial source extract cannot certify a whole country ring.
    if (sourceBounds.IsRectInside(border.GetRect()))
      coverage.push_back(border.Data());
  }
  if (coverage.empty())
    return false;

  std::set<uint64_t> gradeSeparated;
  {
    ReaderSource<FileReader> source{FileReader(info.GetIntermediateFileName(kWaterGradeSeparatedFilename))};
    while (source.Size() != 0)
      gradeSeparated.insert(ReadPrimitiveFromSource<uint64_t>(source));
  }

  cache::IntermediateDataObjectsCache objects;
  cache::IntermediateData data(objects, info);
  std::vector<water::Record> records;
  auto const featuresFile = info.GetTmpFileName(name, ".water_features");
  if (Platform::IsFileExistsByFullPath(featuresFile))
    feature::ForEachFeatureRawFormat(featuresFile, [&](feature::FeatureBuilder const & fb, uint64_t)
    {
      bool const rawCoast = water::IsCoastline(fb.GetTypes()) && !fb.IsCoastCell();
      if (rawCoast && coastsIncluded)
        return;
      bool const area = fb.IsArea() && water::IsArea(fb.GetTypes());
      bool const line = !area && !fb.IsPoint() && water::IsLine(fb.GetTypes());
      if (!line && !area && !rawCoast)
        return;
      water::Record record;
      record.m_rings.assign(fb.GetGeometry().begin(), fb.GetGeometry().end());
      record.m_kind = area ? water::Kind::Area : water::Kind::Line;
      // An OSM coastline encloses land. Only generated, validated ocean cells
      // describe water. Without those cells, shore queries are unsupported.
      if (rawCoast)
        record.m_kind = water::Kind::Unsupported;
      if ((fb.GetParams().layer != feature::LAYER_EMPTY && fb.GetParams().layer != 0) ||
          (fb.HasOsmIds() && gradeSeparated.contains(fb.GetMostGenericOsmId().GetEncodedId())))
        record.m_kind = water::Kind::Unsupported;
      if (line)
      {
        auto const id = fb.GetMostGenericOsmId();
        WayElement way(id.GetSerialId());
        if ((id.GetType() != base::GeoObjectId::Type::OsmWay &&
             id.GetType() != base::GeoObjectId::Type::ObsoleteOsmWay) ||
            !data.GetCache()->GetWay(id.GetSerialId(), way))
          record.m_kind = water::Kind::Unsupported;
        else
        {
          // Read original coordinates and ids together. Display simplification,
          // reversal or feature merging must never alter waterway topology.
          record.m_rings.assign(1, {});
          record.m_nodes = way.m_nodes;
          for (auto const node : way.m_nodes)
          {
            m2::PointD p;
            if (!data.GetCache()->GetNode(node, p.y, p.x))
            {
              record.m_kind = water::Kind::Unsupported;
              record.m_rings.assign(fb.GetGeometry().begin(), fb.GetGeometry().end());
              break;
            }
            record.m_rings.front().push_back(p);
          }
        }
      }
      records.push_back(std::move(record));
    });
  FilesContainerW container(info.GetTargetFileName(name), FileWriter::OP_WRITE_EXISTING);
  auto writer = container.GetWriter(water::kSectionTag);
  auto const start = writer->Pos();
  water::WriteSection(*writer, coverage, records);
  LOG(LINFO, ("Water geometry section:", name, records.size(), "records,", writer->Pos() - start, "bytes"));
  return true;
}
}  // namespace generator
