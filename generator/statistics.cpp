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
  void FileContainerStatistic(std::ostream & os, string const & fPath)
  {
    try
    {
      FilesContainerR cont(fPath);
      cont.ForEachTag([&] (FilesContainerR::Tag const & tag)
      {
        os << std::setw(10) << tag << " : " << cont.GetReader(tag).Size() << '\n';
      });
    }
    catch (Reader::Exception const & ex)
    {
      LOG(LWARNING, ("Error reading file:", fPath, ex.Msg()));
    }
  }

  // 0.001 deg² ≈ 12.392 km² * cos(lat)
  double arrAreas[] = { 10, 20, 50, 100, 200, 500, 1000, 360*360*12400 };

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

      FeatureType::InnerGeomStat const innerStats = f.GetInnerStatistic();

      m_info.m_inner[0].Add(innerStats.m_points);
      m_info.m_inner[1].Add(innerStats.m_strips);
      m_info.m_inner[2].Add(innerStats.m_size);

      // get geometry size for the best geometry
      FeatureType::GeomStat const geom = f.GetGeometrySize(FeatureType::BEST_GEOMETRY);
      FeatureType::GeomStat const trg = f.GetTrianglesSize(FeatureType::BEST_GEOMETRY);

      m_info.m_byPointsCount[CountType(geom.m_count)].Add(geom.m_size);
      m_info.m_byTrgCount[CountType(trg.m_count / 3)].Add(trg.m_size);

      uint32_t const allSize = innerStats.m_size + geom.m_size + trg.m_size;

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

      m_info.m_byAreaSize[AreaType(GetAreaIndex(area))].Add(trg.m_size, len, area, hasName);
    }
  };

  void CalcStatistic(std::string const & fPath, MapInfo & info)
  {
    AccumulateStatistic doProcess(info);
    feature::ForEachFeature(fPath, doProcess);
  }

  void PrintInfo(std::ostream & os, std::string const & prefix, GeneralInfo const & info, bool measurements)
  {
    os << prefix << ": size = " << info.m_size << "; count = " << info.m_count;

    if (measurements)
    {
      os << "; length = " << static_cast<uint64_t>(info.m_length)
         << " m; area = " << static_cast<uint64_t>(info.m_area) << " m²";
    }

    os << "; names = " << info.m_names << '\n';
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
    return classif().GetFullObjectName(t.m_val);
  }

  std::string GetKey(AreaType t)
  {
    return strings::to_string(arrAreas[t.m_val]);
  }

  template <class TSortCr, class TSet>
  void PrintTop(std::ostream & os, char const * prefix, TSet const & theSet)
  {
    os << prefix << endl;

    vector<pair<typename TSet::key_type, typename TSet::mapped_type>> vec(theSet.begin(), theSet.end());

    sort(vec.begin(), vec.end(), TSortCr());

    size_t const count = min(static_cast<size_t>(10), vec.size());
    for (size_t i = 0; i < count; ++i)
    {
      os << i << ". ";
      PrintInfo(os, GetKey(vec[i].first), vec[i].second, false);
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

  void PrintStatistic(std::ostream & os, MapInfo & info)
  {
    PrintInfo(os, "DAT header", info.m_inner[2], false);
    PrintInfo(os, "Points header", info.m_inner[0], false);
    PrintInfo(os, "Strips header", info.m_inner[1], false);

    PrintTop<greater_size>(os, "Top SIZE by Geometry Type", info.m_byGeomType);
    PrintTop<greater_size>(os, "Top SIZE by Classificator Type", info.m_byClassifType);
    PrintTop<greater_size>(os, "Top SIZE by Points Count", info.m_byPointsCount);
    PrintTop<greater_size>(os, "Top SIZE by Triangles Count", info.m_byTrgCount);
    PrintTop<greater_size>(os, "Top SIZE by Area", info.m_byAreaSize);
  }

  void PrintTypeStatistic(std::ostream & os, MapInfo & info)
  {
    for (auto it = info.m_byClassifType.begin(); it != info.m_byClassifType.end(); ++it)
    {
      PrintInfo(os, GetKey(it->first).c_str(), it->second, true);
    }
  }
}
