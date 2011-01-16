#include "../../base/SRC_FIRST.hpp"

#include "statistics.hpp"

#include "../feature_processor.hpp"
#include "../classificator.hpp"
#include "../feature_impl.hpp"

#include "../../base/string_utils.hpp"

#include "../../std/iostream.hpp"

#include "../../base/start_mem_debug.hpp"


namespace stats
{
  void FileContainerStatistic(string const & fName)
  {
    FilesContainerR cont(fName);

    vector<string> tags;
    tags.push_back(DATA_FILE_TAG);
    for (int i = 0; i < ARRAY_SIZE(feature::g_arrScales); ++i)
    {
      tags.push_back(feature::GetTagForIndex(GEOMETRY_FILE_TAG, i));
      tags.push_back(feature::GetTagForIndex(TRIANGLE_FILE_TAG, i));
    }
    tags.push_back(INDEX_FILE_TAG);

    for (size_t i = 0; i < tags.size(); ++i)
      cout << tags[i] << " : " << cont.GetReader(tags[i]).Size() << endl;
  }

  class AccumulateStatistic
  {
    MapInfo & m_info;

    class ProcessType
    {
      MapInfo & m_info;
      uint32_t m_size;

    public:
      ProcessType(MapInfo & info, uint32_t sz) : m_info(info), m_size(sz) {}
      void operator() (uint32_t type)
      {
        m_info.AddToSet(TypeTag(type), m_size, m_info.m_byClassifType);
      }
    };

  public:
    AccumulateStatistic(MapInfo & info) : m_info(info) {}

    void operator() (FeatureType const & f, uint32_t)
    {
      f.ParseBeforeStatistic();

      uint32_t const datSize = f.GetAllSize();
      m_info.m_all.Add(datSize);
      //m_info.m_names.Add(f.GetNameSize());
      //m_info.m_types.Add(f.GetTypesSize());

      FeatureType::geom_stat_t geom = f.GetGeometrySize(-1);
      FeatureType::geom_stat_t trg = f.GetTrianglesSize(-1);

      m_info.AddToSet(geom.m_count, geom.m_size, m_info.m_byPointsCount);
      m_info.AddToSet(trg.m_count / 3, trg.m_size, m_info.m_byTrgCount);

      uint32_t const allSize = datSize + geom.m_size + trg.m_size;

      m_info.AddToSet(f.GetFeatureType(), allSize, m_info.m_byGeomType);

      ProcessType doProcess(m_info, allSize);
      f.ForEachTypeRef(doProcess);
    }
  };

  void CalcStatistic(string const & fName, MapInfo & info)
  {
    AccumulateStatistic doProcess(info);
    feature::ForEachFromDat(fName, doProcess);
  }

  void PrintInfo(char const * prefix, GeneralInfo const & info)
  {
    cout << prefix << ": size = " << info.m_size << "; count = " << info.m_count << endl;
  }

  string GetKey(FeatureBase::FeatureType type)
  {
    switch (type)
    {
    case FeatureBase::FEATURE_TYPE_LINE: return "Line";
    case FeatureBase::FEATURE_TYPE_AREA: return "Area";
    default: return "Point";
    }
  }

  string GetKey(uint32_t i)
  {
    return utils::to_string(i);
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
    PrintInfo("DAT", info.m_all);
    //PrintInfo("NAMES", info.m_names);
    //PrintInfo("TYPES", info.m_types);

    PrintTop<greater_size>("Top SIZE by Geometry Type", info.m_byGeomType);
    PrintTop<greater_size>("Top SIZE by Classificator Type", info.m_byClassifType);
    PrintTop<greater_size>("Top SIZE by Points Count", info.m_byPointsCount);
    PrintTop<greater_size>("Top SIZE by Triangles Count", info.m_byTrgCount);
  }
}
