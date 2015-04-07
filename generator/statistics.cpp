#include "base/SRC_FIRST.hpp"

#include "generator/statistics.hpp"

#include "indexer/feature_processor.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/data_factory.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include "std/iostream.hpp"
#include "std/iomanip.hpp"


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

      m_info.AddToSet(geom.m_count, geom.m_size, m_info.m_byPointsCount);
      m_info.AddToSet(trg.m_count / 3, trg.m_size, m_info.m_byTrgCount);

      uint32_t const allSize = innerStats.m_size + geom.m_size + trg.m_size;

      m_info.AddToSet(f.GetFeatureType(), allSize, m_info.m_byGeomType);

      f.ForEachType([this, allSize](uint32_t type)
      {
        m_info.AddToSet(TypeTag(type), allSize, m_info.m_byClassifType);
      });
    }
  };

  void CalcStatistic(string const & fPath, MapInfo & info)
  {
    AccumulateStatistic doProcess(info);
    feature::ForEachFromDat(fPath, doProcess);
  }

  void PrintInfo(char const * prefix, GeneralInfo const & info)
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

  string GetKey(uint32_t i)
  {
    return strings::to_string(i);
  }

  string GetKey(TypeTag t)
  {
    return classif().GetFullObjectName(t.m_val);
  }

  template <class TSortCr, class TSet>
  void PrintTop(char const * prefix, TSet const & theSet)
  {
    cout << prefix << endl;

    vector<typename TSet::value_type> vec(theSet.begin(), theSet.end());

    sort(vec.begin(), vec.end(), TSortCr());

    size_t const count = min(static_cast<size_t>(10), vec.size());
    for (size_t i = 0; i < count; ++i)
    {
      cout << i << ". ";
      PrintInfo(GetKey(vec[i].m_key).c_str(), vec[i].m_info);
    }
  }

  struct greater_size
  {
    template <class TInfo>
    bool operator() (TInfo const & r1, TInfo const & r2) const
    {
      return r1.m_info.m_size > r2.m_info.m_size;
    }
  };

  struct greater_count
  {
    template <class TInfo>
    bool operator() (TInfo const & r1, TInfo const & r2) const
    {
      return r1.m_info.m_count > r2.m_info.m_count;
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
  }
}
