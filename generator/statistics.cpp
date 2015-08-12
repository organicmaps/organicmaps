#include "statistics.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_factory.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_processor.hpp"

#include "geometry/triangle2d.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/iomanip.hpp"
#include "std/iostream.hpp"


using namespace feature;

namespace stats
{
  void FileContainerStatistic(string const & fPath)
  {
    try
    {
      FilesContainerR cont(fPath);
      cont.ForEachTag([&cont] (FilesContainerR::Tag const & tag)
      {
        cout << setw(10) << tag << " : " << cont.GetReader(tag).Size() << endl;
      });
    }
    catch (Reader::Exception const & ex)
    {
      LOG(LWARNING, ("Error reading file:", fPath, ex.Msg()));
    }
  }

  double arrSquares[] = { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 360*360 };

  size_t GetSquareIndex(double s)
  {
    auto end = arrSquares + ARRAY_SIZE(arrSquares);
    auto i = lower_bound(arrSquares, end, s);
    ASSERT(i != end, ());
    return distance(arrSquares, i);
  }

  class AccumulateStatistic
  {
    MapInfo & m_info;

  public:
    AccumulateStatistic(MapInfo & info) : m_info(info) {}

    void operator() (FeatureType const & f, uint32_t)
    {
      f.ParseBeforeStatistic();

      FeatureType::inner_geom_stat_t const innerStats = f.GetInnerStatistic();

      m_info.m_inner[0].Add(innerStats.m_points);
      m_info.m_inner[1].Add(innerStats.m_strips);
      m_info.m_inner[2].Add(innerStats.m_size);

      // get geometry size for the best geometry
      FeatureType::geom_stat_t const geom = f.GetGeometrySize(FeatureType::BEST_GEOMETRY);
      FeatureType::geom_stat_t const trg = f.GetTrianglesSize(FeatureType::BEST_GEOMETRY);

      m_info.AddToSet(CountType(geom.m_count), geom.m_size, m_info.m_byPointsCount);
      m_info.AddToSet(CountType(trg.m_count / 3), trg.m_size, m_info.m_byTrgCount);

      uint32_t const allSize = innerStats.m_size + geom.m_size + trg.m_size;

      m_info.AddToSet(f.GetFeatureType(), allSize, m_info.m_byGeomType);

      f.ForEachType([this, allSize](uint32_t type)
      {
        m_info.AddToSet(ClassifType(type), allSize, m_info.m_byClassifType);
      });

      double square = 0.0;
      f.ForEachTriangle([&square](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
      {
        square += m2::GetTriangleArea(p1, p2, p3);
      }, FeatureType::BEST_GEOMETRY);

      m_info.AddToSet(AreaType(GetSquareIndex(square)), trg.m_size, m_info.m_byAreaSize);
    }
  };

  void CalcStatistic(string const & fPath, MapInfo & info)
  {
    AccumulateStatistic doProcess(info);
    feature::ForEachFromDat(fPath, doProcess);
  }

  void PrintInfo(string const & prefix, GeneralInfo const & info)
  {
    cout << prefix << ": size = " << info.m_size << "; count = " << info.m_count << endl;
  }

  string GetKey(EGeomType type)
  {
    switch (type)
    {
    case GEOM_LINE: return "Line";
    case GEOM_AREA: return "Area";
    default: return "Point";
    }
  }

  string GetKey(CountType t)
  {
    return strings::to_string(t.m_val);
  }

  string GetKey(ClassifType t)
  {
    return classif().GetFullObjectName(t.m_val);
  }

  string GetKey(AreaType t)
  {
    return strings::to_string(arrSquares[t.m_val]);
  }

  template <class TSortCr, class TSet>
  void PrintTop(char const * prefix, TSet const & theSet)
  {
    cout << prefix << endl;

    vector<pair<typename TSet::key_type, typename TSet::mapped_type>> vec(theSet.begin(), theSet.end());

    sort(vec.begin(), vec.end(), TSortCr());

    size_t const count = min(static_cast<size_t>(10), vec.size());
    for (size_t i = 0; i < count; ++i)
    {
      cout << i << ". ";
      PrintInfo(GetKey(vec[i].first), vec[i].second);
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

  void PrintStatistic(MapInfo & info)
  {
    PrintInfo("DAT header", info.m_inner[2]);
    PrintInfo("Points header", info.m_inner[0]);
    PrintInfo("Strips header", info.m_inner[1]);

    PrintTop<greater_size>("Top SIZE by Geometry Type", info.m_byGeomType);
    PrintTop<greater_size>("Top SIZE by Classificator Type", info.m_byClassifType);
    PrintTop<greater_size>("Top SIZE by Points Count", info.m_byPointsCount);
    PrintTop<greater_size>("Top SIZE by Triangles Count", info.m_byTrgCount);
    PrintTop<greater_size>("Top SIZE by Square", info.m_byAreaSize);
  }
}
