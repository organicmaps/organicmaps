#include "statistics.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_processor.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <iterator>

namespace stats
{
using namespace feature;

void PrintFileContainerStats(std::ostream & os, std::string const & fPath)
{
  os << "File section sizes\n";
  try
  {
    FilesContainerR cont(fPath);
    cont.ForEachTag([&](FilesContainerR::Tag const & tag)
    { os << std::setw(18) << tag << " : " << std::setw(10) << cont.GetReader(tag).Size() << "\n"; });
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Error reading file:", fPath, ex.Msg()));
  }
  os << "\n";
}

// 0.001 deg² ≈ 12.392 km² * cos(lat)
static constexpr double kAreas[] = {10, 20, 50, 100, 200, 500, 1000, 5000, 360 * 360 * 12400};

size_t GetAreaIndex(double s)
{
  double const sInKm2 = s / 1000000;
  auto const i = std::lower_bound(std::begin(kAreas), std::end(kAreas), sInKm2);
  ASSERT(i != std::end(kAreas), ());
  return std::distance(kAreas, i);
}

class AccumulateStatistic
{
  MapInfo & m_info;

public:
  explicit AccumulateStatistic(MapInfo & info) : m_info(info) {}

  void operator()(FeatureType & f, uint32_t)
  {
    f.ParseAllBeforeGeometry();

    FeatureType::InnerGeomStat const innerStats = f.GetInnerStats();

    m_info.m_innerPoints.Add(innerStats.m_points);
    m_info.m_innerFirstPoints.Add(innerStats.m_firstPoints);
    m_info.m_innerStrips.Add(innerStats.m_strips);
    m_info.m_innerSize.Add(innerStats.m_size);

    // Get size stats and load the best geometry.
    FeatureType::GeomStat const geom = f.GetOuterGeometryStats();
    FeatureType::GeomStat const trg = f.GetOuterTrianglesStats();

    uint32_t outerGeomSize = 0, outerTrgSize = 0;
    for (size_t ind = 0; ind < DataHeader::kMaxScalesCount; ++ind)
    {
      auto const geomSize = geom.m_sizes[ind], geomElems = geom.m_elements[ind], trgSize = trg.m_sizes[ind],
                 trgElems = trg.m_elements[ind];
      m_info.m_byLineGeom[ind].Add(geomSize, geomElems);
      outerGeomSize += geomSize;
      m_info.m_byAreaGeom[ind].Add(trgSize, trgElems);
      outerTrgSize += trgSize;

      if (ind > 0)
      {
        // If a feature has a more simplified version of current geometry.
        if (geom.m_elements[ind - 1] > 0)
          m_info.m_byLineGeomComparedS[ind].Add(geomSize, geomElems);
        if (trg.m_elements[ind - 1] > 0)
          m_info.m_byAreaGeomComparedS[ind].Add(trgSize, trgElems);
      }

      if (ind < DataHeader::kMaxScalesCount - 1)
      {
        // If a feature has a more detailed version of current geometry.
        if (geom.m_elements[ind + 1] > 0)
        {
          m_info.m_byLineGeomComparedD[ind].Add(geomSize, geomElems);
          // If feature's current geometry is very similar to a more detailed one
          // (number of elements is <geomMinDiff less or <geomMinFactor times less).
          if (geomElems + m_info.m_geomMinDiff > geom.m_elements[ind + 1] ||
              geomElems * m_info.m_geomMinFactor > geom.m_elements[ind + 1])
            m_info.m_byLineGeomDup[ind].Add(geomSize, geomElems);
        }
        if (trg.m_elements[ind + 1] > 0)
        {
          m_info.m_byAreaGeomComparedD[ind].Add(trgSize, trgElems);
          if (trgElems + m_info.m_geomMinDiff > trg.m_elements[ind + 1] ||
              trgElems * m_info.m_geomMinFactor > trg.m_elements[ind + 1])
            m_info.m_byAreaGeomDup[ind].Add(trgSize, trgElems);
        }
      }
    }

    m_info.m_byPointsCount[CountType(geom.m_elements[DataHeader::kMaxScalesCount - 1])].Add(innerStats.m_points +
                                                                                            outerGeomSize);
    m_info.m_byTrgCount[CountType(trg.m_elements[DataHeader::kMaxScalesCount - 1])].Add(innerStats.m_strips +
                                                                                        outerTrgSize);

    // Header size (incl. inner geometry) + outer geometry size.
    uint32_t const allSize = innerStats.m_size + outerGeomSize + outerTrgSize;

    double len = 0.0;
    double area = 0.0;

    if (f.GetGeomType() == feature::GeomType::Line)
    {
      m2::PointD lastPoint;
      bool firstPoint = true;
      f.ForEachPoint([&len, &firstPoint, &lastPoint](m2::PointD const & pt)
      {
        if (firstPoint)
          firstPoint = false;
        else
          len += mercator::DistanceOnEarth(lastPoint, pt);
        lastPoint = pt;
      }, FeatureType::BEST_GEOMETRY);
    }
    else if (f.GetGeomType() == feature::GeomType::Area)
    {
      f.ForEachTriangle([&area](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
      { area += mercator::AreaOnEarth(p1, p2, p3); }, FeatureType::BEST_GEOMETRY);
    }

    auto const hasName = f.GetNames().CountLangs() != 0;

    m_info.m_byGeomType[f.GetGeomType()].Add(allSize, len, area, hasName);

    f.ForEachType([this, allSize, len, area, hasName](uint32_t type)
    { m_info.m_byClassifType[ClassifType(type)].Add(allSize, len, area, hasName); });

    m_info.m_byAreaSize[AreaType(GetAreaIndex(area))].Add(allSize, len, area, hasName);
  }
};

void CalcStats(std::string const & fPath, MapInfo & info)
{
  AccumulateStatistic doProcess(info);
  feature::ForEachFeature(fPath, doProcess);
}

void PrintInfo(std::ostream & os, std::string const & prefix, GeneralInfo const & info, uint8_t prefixWidth = 1,
               bool names = false, bool measurements = false)
{
  os << std::setw(prefixWidth) << prefix << ": size = " << std::setw(9) << info.m_size
     << "; features = " << std::setw(7) << info.m_count << "; bytes/feats = " << std::setw(6)
     << info.m_size / static_cast<double>(info.m_count);

  if (measurements)
  {
    os << "; length = " << std::setw(10) << static_cast<uint64_t>(info.m_length) << " m; area = " << std::setw(10)
       << static_cast<uint64_t>(info.m_area) << " m²";
  }
  if (names)
    os << "; w/names = " << std::setw(8) << info.m_names;

  os << "\n";
}

std::string GetKey(GeomType type)
{
  switch (type)
  {
  case GeomType::Line: return "Line";
  case GeomType::Area: return "Area";
  default: return "Point";
  }
}

std::string GetKey(CountType t)
{
  return strings::to_string(t.m_val);
}

std::string GetKey(ClassifType t)
{
  return classif().GetReadableObjectName(t.m_val);
}

std::string GetKey(AreaType t)
{
  return strings::to_string(kAreas[t.m_val]);
}

template <class TSortCr, class TSet>
void PrintTop(std::ostream & os, char const * prefix, TSet const & theSet, uint8_t prefixWidth = 5, bool names = false)
{
  os << "\n" << prefix << "\n";

  std::vector<std::pair<typename TSet::key_type, typename TSet::mapped_type>> vec(theSet.begin(), theSet.end());

  sort(vec.begin(), vec.end(), TSortCr());

  size_t const count = std::min(static_cast<size_t>(20), vec.size());
  for (size_t i = 0; i < count; ++i)
  {
    os << std::setw(2) << i << ". ";
    PrintInfo(os, GetKey(vec[i].first), vec[i].second, prefixWidth, names);
  }
}

struct greater_size
{
  template <class TInfo>
  bool operator()(TInfo const & r1, TInfo const & r2) const
  {
    return r1.second.m_size > r2.second.m_size;
  }
};

struct greater_count
{
  template <class TInfo>
  bool operator()(TInfo const & r1, TInfo const & r2) const
  {
    return r1.second.m_count > r2.second.m_count;
  }
};

void PrintStats(std::ostream & os, MapInfo & info)
{
  PrintInfo(os, "Feature headers", info.m_innerSize, 30);
  PrintInfo(os, "incl. inner points", info.m_innerPoints, 30);
  PrintInfo(os, "incl. first/base outer points", info.m_innerFirstPoints, 30);
  PrintInfo(os, "incl. inner triangles (strips)", info.m_innerStrips, 30);

  PrintTop<greater_size>(os, "Top SIZE by Geometry Type", info.m_byGeomType, 5, true);
  PrintTop<greater_size>(os,
                         "Top SIZE by Classificator Type\n"
                         "(a single feature's size may be included in several types)",
                         info.m_byClassifType, 30, true);
  PrintTop<greater_size>(os, "Top SIZE by Points Count", info.m_byPointsCount);
  PrintTop<greater_size>(os, "Top SIZE by Triangles Count", info.m_byTrgCount);
  PrintTop<greater_size>(os, "Top SIZE by Area", info.m_byAreaSize, 5, true);
  os << "\n";
}

/// @note If you gonna change this function, take into account
/// ./tools/python/maps_generator/generator/statistics.py
void PrintTypeStats(std::ostream & os, MapInfo & info)
{
  os << "Feature stats by Classificator Type\n"
     << "(a single feature can contain several types and thus its size can be included in several type lines)\n";

  for (auto const & e : info.m_byClassifType)
    PrintInfo(os, GetKey(e.first), e.second, 30, true, true);

  os << "\n";
}

void PrintGeometryInfo(std::ostream & os, char const * prefix, size_t geomMinDiff, double geomMinFactor,
                       GeomStats const & geomStats, GeomStats const & comparedStatsS, GeomStats const & comparedStatsD,
                       GeomStats const & dupStats)
{
  for (size_t ind = 0; ind < DataHeader::kMaxScalesCount; ++ind)
  {
    GeomInfo const & info = geomStats[ind];
    os << "    " << prefix << ind << ": size = " << std::setw(9) << info.m_size << ": elements = " << std::setw(9)
       << info.m_elements << "; features = " << std::setw(7) << info.m_count << "; elems/feats = " << std::setw(5)
       << info.m_elements / static_cast<double>(info.m_count) << "; bytes/elems = " << std::setw(4)
       << info.m_size / static_cast<double>(info.m_elements) << "\n";
  }

  os << "Geometry of features present on adjacent geom scales incl. very similar geometries\n"
     << "(when number of elements is <" << geomMinDiff << " less or <" << geomMinFactor << "x times less)\n";
  for (size_t ind = 0; ind < DataHeader::kMaxScalesCount - 1; ++ind)
  {
    GeomInfo const &compInfoD = comparedStatsD[ind], compInfoS = comparedStatsS[ind + 1], dupInfo = dupStats[ind];
    CHECK_EQUAL(compInfoD.m_count, compInfoS.m_count, ());
    os << prefix << ind << " vs " << prefix << ind + 1 << ": size difference = " << std::setw(4)
       << compInfoS.m_size / static_cast<double>(compInfoD.m_size) << "x; elems difference = " << std::setw(4)
       << compInfoS.m_elements / static_cast<double>(compInfoD.m_elements) << "x; features = " << std::setw(7)
       << compInfoD.m_count << "\n";
    os << "    " << prefix << ind << ": size = " << std::setw(9) << compInfoD.m_size << "; elements = " << std::setw(9)
       << compInfoD.m_elements << "; elems/feats = " << std::setw(5)
       << compInfoD.m_elements / static_cast<double>(compInfoD.m_count) << "\n";
    os << "    " << prefix << ind + 1 << ": size = " << std::setw(9) << compInfoS.m_size
       << "; elements = " << std::setw(9) << compInfoS.m_elements << "; elems/feats = " << std::setw(5)
       << compInfoS.m_elements / static_cast<double>(compInfoS.m_count) << "\n";
    os << "    similar: size = " << std::setw(9) << dupInfo.m_size << " (" << std::setw(2)
       << 100 * dupInfo.m_size / static_cast<double>(compInfoD.m_size) << "%); elements = " << std::setw(9)
       << dupInfo.m_elements << "; features = " << std::setw(7) << dupInfo.m_count << "; elems/feats = " << std::setw(5)
       << dupInfo.m_elements / static_cast<double>(dupInfo.m_count) << "\n";
  }
}

void PrintOuterGeometryStats(std::ostream & os, MapInfo & info)
{
  os << "Outer LINE geometry\n";
  PrintGeometryInfo(os, "geom", info.m_geomMinDiff, info.m_geomMinFactor, info.m_byLineGeom, info.m_byLineGeomComparedS,
                    info.m_byLineGeomComparedD, info.m_byLineGeomDup);

  os << "\nOuter AREA geometry\n";
  PrintGeometryInfo(os, "trg", info.m_geomMinDiff, info.m_geomMinFactor, info.m_byAreaGeom, info.m_byAreaGeomComparedS,
                    info.m_byAreaGeomComparedD, info.m_byAreaGeomDup);
  os << "\n";
}
}  // namespace stats
