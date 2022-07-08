#include "statistics.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_factory.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_processor.hpp"

#include "geometry/triangle2d.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <iterator>

using namespace feature;
using namespace std;

namespace stats
{
  static const double kAlmostDupElemsFactor = 1.5;

  void FileContainerStatistics(std::ostream & os, string const & fPath)
  {
    try
    {
      os << "File section sizes" << endl;
      FilesContainerR cont(fPath);
      cont.ForEachTag([&] (FilesContainerR::Tag const & tag)
      {
        os << std::setw(18) << tag << " : "
           << std::setw(10) << cont.GetReader(tag).Size() << endl;
      });
    }
    catch (Reader::Exception const & ex)
    {
      LOG(LWARNING, ("Error reading file:", fPath, ex.Msg()));
    }
    os << endl;
  }

  // 0.001 deg² ≈ 12.392 km² * cos(lat)
  double arrAreas[] = { 10, 20, 50, 100, 200, 500, 1000, 5000, 360*360*12400 };

  size_t GetAreaIndex(double s)
  {
    double const sInKm2 = s / 1000000;
    auto i = lower_bound(std::begin(arrAreas), std::end(arrAreas), sInKm2);
    ASSERT(i != std::end(arrAreas), ());
    return distance(arrAreas, i);
  }

  class AccumulateStatistic
  {
    MapInfo & m_info;

  public:
    explicit AccumulateStatistic(MapInfo & info) : m_info(info) {}

    void operator() (FeatureType & f, uint32_t)
    {
      f.ParseBeforeStatistic();

      FeatureType::InnerGeomStat const innerStats = f.GetInnerStats();

      m_info.m_inner[0].Add(innerStats.m_points);
      m_info.m_inner[1].Add(innerStats.m_strips);
      m_info.m_inner[2].Add(innerStats.m_size);

      // Get size stats and load the best geometry.
      FeatureType::GeomStat const geom = f.GetOuterGeometryStats();
      FeatureType::GeomStat const trg = f.GetOuterTrianglesStats();

      uint32_t geomSize = 0, trgSize = 0;
      int const n = feature::DataHeader::kMaxScalesCount;
      for (int ind = 0; ind < n; ++ind)
      {
        m_info.m_byLineGeom[ind].Add(geom.m_sizes[ind], geom.m_elements[ind]);
        geomSize += geom.m_sizes[ind];
        m_info.m_byAreaGeom[ind].Add(trg.m_sizes[ind], trg.m_elements[ind]);
        trgSize += trg.m_sizes[ind];

        if (ind > 0)
        {
          // If a feature has a more simplified version of current geometry.
          if (geom.m_elements[ind - 1] > 0)
            m_info.m_byLineGeomCompared[ind].Add(geom.m_sizes[ind], geom.m_elements[ind]);
          if (trg.m_elements[ind - 1] > 0)
            m_info.m_byAreaGeomCompared[ind].Add(trg.m_sizes[ind], trg.m_elements[ind]);
        }

        if (ind < n - 1)
        {
          // If feature's current geometry almost duplicates a more detailed one
          // (has <kAlmostDupElemsFactor less elements).
          if (geom.m_elements[ind] > 0 && geom.m_elements[ind] * kAlmostDupElemsFactor > geom.m_elements[ind + 1])
            m_info.m_byLineGeomDup[ind].Add(geom.m_sizes[ind], geom.m_elements[ind]);
          if (trg.m_elements[ind] > 0 && trg.m_elements[ind] * kAlmostDupElemsFactor > trg.m_elements[ind + 1])
            m_info.m_byAreaGeomDup[ind].Add(trg.m_sizes[ind], trg.m_elements[ind]);
        }
      }

      m_info.m_byPointsCount[CountType(geom.m_elements[n - 1])].Add(innerStats.m_points + geomSize);
      m_info.m_byTrgCount[CountType(trg.m_elements[n - 1])].Add(innerStats.m_strips + trgSize);

      // Header size (incl. inner geometry) + outer geometry size.
      uint32_t const allSize = innerStats.m_size + geomSize + trgSize;

      double len = 0.0;
      double area = 0.0;

      if (f.GetGeomType() == feature::GeomType::Line)
      {
        m2::PointD lastPoint;
        bool firstPoint = true;
        f.ForEachPoint([&len, &firstPoint, &lastPoint](const m2::PointD & pt)
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
        {
          area += mercator::AreaOnEarth(p1, p2, p3);
        }, FeatureType::BEST_GEOMETRY);
      }

      auto const hasName = f.GetNames().CountLangs() != 0;

      m_info.m_byGeomType[f.GetGeomType()].Add(allSize, len, area, hasName);

      f.ForEachType([this, allSize, len, area, hasName](uint32_t type)
      {
        m_info.m_byClassifType[ClassifType(type)].Add(allSize, len, area, hasName);
      });

      m_info.m_byAreaSize[AreaType(GetAreaIndex(area))].Add(allSize, len, area, hasName);
    }
  };

  void CalcStatistics(std::string const & fPath, MapInfo & info)
  {
    AccumulateStatistic doProcess(info);
    feature::ForEachFeature(fPath, doProcess);
  }

  void PrintInfo(std::ostream & os, std::string const & prefix,
                 GeneralInfo const & info, uint8_t prefixWidth = 1,
                 bool names = false, bool measurements = false)
  {
    os << std::setw(prefixWidth) << prefix
       << ": size = " << std::setw(9) << info.m_size
       << "; features = " << std::setw(7) << info.m_count;

    if (measurements)
    {
      os << "; length = " << std::setw(10) << static_cast<uint64_t>(info.m_length)
         << " m; area = " << std::setw(10) << static_cast<uint64_t>(info.m_area) << " m²";
    }
    if (names)
      os << "; w/names = " << std::setw(8) << info.m_names;

    os << endl;
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
    return strings::to_string(arrAreas[t.m_val]);
  }

  template <class TSortCr, class TSet>
  void PrintTop(std::ostream & os, char const * prefix, TSet const & theSet,
                uint8_t prefixWidth = 5, bool names = false)
  {
    os << endl << prefix << endl;

    vector<pair<typename TSet::key_type, typename TSet::mapped_type>> vec(theSet.begin(), theSet.end());

    sort(vec.begin(), vec.end(), TSortCr());

    size_t const count = min(static_cast<size_t>(10), vec.size());
    for (size_t i = 0; i < count; ++i)
    {
      os << std::setw(2) << i << ". ";
      PrintInfo(os, GetKey(vec[i].first), vec[i].second, prefixWidth, names);
    }
  }

  struct greater_size
  {
    template <class TInfo>
    bool operator() (TInfo const & r1, TInfo const & r2) const
    {
      return r1.second.m_size > r2.second.m_size;
    }
  };

  struct greater_count
  {
    template <class TInfo>
    bool operator() (TInfo const & r1, TInfo const & r2) const
    {
      return r1.second.m_count > r2.second.m_count;
    }
  };

  void PrintStatistics(std::ostream & os, MapInfo & info)
  {
    PrintInfo(os, "Feature headers", info.m_inner[2], 30);
    PrintInfo(os, "incl. inner points", info.m_inner[0], 30);
    PrintInfo(os, "incl. inner triangles (strips)", info.m_inner[1], 30);

    PrintTop<greater_size>(os, "Top SIZE by Geometry Type", info.m_byGeomType, 5, true);
    PrintTop<greater_size>(os, "Top SIZE by Classificator Type\n"
                           "(a single feature's size may be included in several types)",
                           info.m_byClassifType, 30, true);
    PrintTop<greater_size>(os, "Top SIZE by Points Count", info.m_byPointsCount);
    PrintTop<greater_size>(os, "Top SIZE by Triangles Count", info.m_byTrgCount);
    PrintTop<greater_size>(os, "Top SIZE by Area", info.m_byAreaSize, 5, true);
    os << endl;
  }

  void PrintTypeStatistics(std::ostream & os, MapInfo & info)
  {
    os << "Feature stats by Classificator Type" << endl
       << "(a single feature can contain several types and thus its size can be included in several type lines)"
       << endl;
    for (auto it = info.m_byClassifType.begin(); it != info.m_byClassifType.end(); ++it)
    {
      PrintInfo(os, GetKey(it->first), it->second, 30, true, true);
    }
    os << endl;
  }

  void PrintGeometryInfo(std::ostream & os, char const * prefix, GeomStats const & geomStats,
                         GeomStats const & comparedStats, GeomStats const & dupStats)
  {
    int const n = feature::DataHeader::kMaxScalesCount;
    for (int ind = 0; ind < n; ++ind)
    {
      GeomInfo const & info = geomStats[ind];
      if (ind > 0)
      {
        GeomInfo const & compInfo = comparedStats[ind];
        os << prefix << ind << "w/" << ind - 1
           << ": size = " << std::setw(9) << compInfo.m_size
           << ": elements = " << std::setw(9) << compInfo.m_elements
           << "; feats w/" << prefix << ind - 1
           << " = " << std::setw(7) << compInfo.m_count
           << "; elems/feats = " << std::setw(5)
           << compInfo.m_elements / static_cast<double>(compInfo.m_count)
           << "; size factor = " << std::setw(4)
           << compInfo.m_size / static_cast<double>(geomStats[ind - 1].m_size)
           << "x; elems factor = " << std::setw(4)
           << compInfo.m_elements / static_cast<double>(geomStats[ind - 1].m_elements)
           << "x" << endl;
      }
      os << "   " << prefix << ind
         << ": size = " << std::setw(9) << info.m_size
         << ": elements = " << std::setw(9) << info.m_elements
         << "; features = " << std::setw(7) << info.m_count
         << "; elems/feats = " << std::setw(5)
         << info.m_elements / static_cast<double>(info.m_count)
         << "; bytes/elems = " << std::setw(4)
         << info.m_size / static_cast<double>(info.m_elements)
         << endl;
    }

    os << "Geometry almost duplicating (<" << kAlmostDupElemsFactor
       << "x less elements) a more detailed one" << endl;
    for (int ind = 0; ind < n - 1; ++ind)
    {
      GeomInfo const & dupInfo = dupStats[ind];
      os << prefix << ind << "~=" << ind + 1
         << ": size = " << std::setw(9) << dupInfo.m_size
         << ": elements = " << std::setw(9) << dupInfo.m_elements
         << "; features = " << std::setw(7) << dupInfo.m_count
         << "; elems/feats = " << std::setw(5)
         << dupInfo.m_elements / static_cast<double>(dupInfo.m_count)
         << "; dups size % = " << std::setw(2)
         << 100 * dupInfo.m_size / geomStats[ind].m_size << "%"
         << endl;
    }
  }

  void PrintOuterGeometryStatistics(std::ostream & os, MapInfo & info)
  {
    os << "Outer LINE geometry" << fixed << setprecision(1) << endl;
    PrintGeometryInfo(os, "geom", info.m_byLineGeom, info.m_byLineGeomCompared, info.m_byLineGeomDup);

    os << endl << "Outer AREA geometry" << endl;
    PrintGeometryInfo(os, "trg", info.m_byAreaGeom, info.m_byAreaGeomCompared, info.m_byAreaGeomDup);
    os << endl;
  }
}
