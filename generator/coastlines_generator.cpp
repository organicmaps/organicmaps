#include "generator/coastlines_generator.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/point_to_int64.hpp"

#include "geometry/region2d/binary_operators.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include "3party/gflags/src/gflags/gflags.h"

#include "std/bind.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/thread.hpp"
#include "std/utility.hpp"

typedef m2::RegionI RegionT;
typedef m2::PointI PointT;
typedef m2::RectI RectT;

DECLARE_bool(fail_on_coasts);

CoastlineFeaturesGenerator::CoastlineFeaturesGenerator(uint32_t coastType,
                                                       int lowLevel, int highLevel, int maxPoints)
  : m_merger(POINT_COORD_BITS), m_coastType(coastType),
    m_lowLevel(lowLevel), m_highLevel(highLevel), m_maxPoints(maxPoints)
{
  ASSERT_LESS_OR_EQUAL ( m_lowLevel, m_highLevel, () );
}

namespace
{
  m2::RectD GetLimitRect(RegionT const & rgn)
  {
    RectT r = rgn.GetRect();
    return m2::RectD(r.minX(), r.minY(), r.maxX(), r.maxY());
  }

  inline PointT D2I(m2::PointD const & p)
  {
    m2::PointU const pu = PointD2PointU(p, POINT_COORD_BITS);
    return PointT(static_cast<int32_t>(pu.x), static_cast<int32_t>(pu.y));
  }

  template <class TreeT> class DoCreateRegion
  {
    TreeT & m_tree;

    RegionT m_rgn;
    m2::PointD m_pt;
    bool m_exist;

  public:
    DoCreateRegion(TreeT & tree) : m_tree(tree), m_exist(false) {}

    bool operator()(m2::PointD const & p)
    {
      // This logic is to skip last polygon point (that is equal to first).

      if (m_exist)
      {
        // add previous point to region
        m_rgn.AddPoint(D2I(m_pt));
      }
      else
        m_exist = true;

      // save current point
      m_pt = p;
      return true;
    }

    void EndRegion()
    {
      m_tree.Add(m_rgn, GetLimitRect(m_rgn));

      m_rgn = RegionT();
      m_exist = false;
    }
  };
}

void CoastlineFeaturesGenerator::AddRegionToTree(FeatureBuilder1 const & fb)
{
  ASSERT ( fb.IsGeometryClosed(), () );

  DoCreateRegion<TTree> createRgn(m_tree);
  fb.ForEachGeometryPointEx(createRgn);
}

void CoastlineFeaturesGenerator::operator()(FeatureBuilder1 const & fb)
{
  if (fb.IsGeometryClosed())
    AddRegionToTree(fb);
  else
    m_merger(fb);
}

namespace
{
  class DoAddToTree : public FeatureEmitterIFace
  {
    CoastlineFeaturesGenerator & m_rMain;
    size_t m_notMergedCoastsCount;
    size_t m_totalNotMergedCoastsPoints;

  public:
    DoAddToTree(CoastlineFeaturesGenerator & rMain)
      : m_rMain(rMain), m_notMergedCoastsCount(0), m_totalNotMergedCoastsPoints(0) {}

    virtual void operator() (FeatureBuilder1 const & fb)
    {
      if (fb.IsGeometryClosed())
        m_rMain.AddRegionToTree(fb);
      else
      {
        LOG(LINFO, ("Not merged coastline", fb.GetOsmIdsString()));
        ++m_notMergedCoastsCount;
        m_totalNotMergedCoastsPoints += fb.GetPointsCount();
      }
    }

    bool HasNotMergedCoasts() const
    {
      return m_notMergedCoastsCount != 0;
    }

    size_t GetNotMergedCoastsCount() const
    {
      return m_notMergedCoastsCount;
    }

    size_t GetNotMergedCoastsPoints() const
    {
      return m_totalNotMergedCoastsPoints;
    }
  };
}

bool CoastlineFeaturesGenerator::Finish()
{
  DoAddToTree doAdd(*this);
  m_merger.DoMerge(doAdd);

  if (doAdd.HasNotMergedCoasts())
  {
    LOG(LINFO, ("Total not merged coasts:", doAdd.GetNotMergedCoastsCount()));
    LOG(LINFO, ("Total points in not merged coasts:", doAdd.GetNotMergedCoastsPoints()));
    if (FLAGS_fail_on_coasts)
      return false;
  }

  return true;
}

namespace
{
  class DoDifference
  {
    RectT m_src;
    vector<RegionT> m_res;
    vector<m2::PointD> m_points;

  public:
    DoDifference(RegionT const & rgn)
    {
      m_res.push_back(rgn);
      m_src = rgn.GetRect();
    }

    void operator() (RegionT const & r)
    {
      if (m_src.IsRectInside(r.GetRect()))
      {
        // if r is fully inside source rect region,
        // put it to the result vector without any intersection
        m_res.push_back(r);
      }
      else
      {
        m2::IntersectRegions(m_res.front(), r, m_res);
      }
    }

    void operator() (PointT const & p)
    {
      m_points.push_back(PointU2PointD(m2::PointU(
                                static_cast<uint32_t>(p.x),
                                static_cast<uint32_t>(p.y)), POINT_COORD_BITS));
    }

    size_t GetPointsCount() const
    {
      size_t count = 0;
      for (size_t i = 0; i < m_res.size(); ++i)
        count += m_res[i].GetPointsCount();
      return count;
    }

    void AssignGeometry(FeatureBuilder1 & fb)
    {
      for (size_t i = 0; i < m_res.size(); ++i)
      {
        m_points.clear();
        m_points.reserve(m_res[i].Size() + 1);

        m_res[i].ForEachPoint(ref(*this));

        fb.AddPolygon(m_points);
      }
    }
  };
}

class RegionInCellSplitter final
{
public:
  typedef RectId TCell;
  typedef m4::Tree<m2::RegionI> TIndex;
  typedef function<void(TCell const &, DoDifference &)> TProcessResultFunc;

protected:
  TIndex const & m_index;
  mutex & m_mutexTasks;
  list<TCell> & m_listTasks;
  list<TCell> m_errorCell;
  condition_variable & m_listCondVar;
  size_t & m_inWork;
  TProcessResultFunc m_processResultFunc;
  mutex & m_mutexResult;


  RegionInCellSplitter(list<TCell> & listTasks, mutex & mutexTasks, condition_variable & condVar, size_t & inWork,
             TProcessResultFunc funcResult, mutex & mutexResult,
             TIndex const & index)
  : m_index(index)
  , m_mutexTasks(mutexTasks)
  , m_listTasks(listTasks)
  , m_listCondVar(condVar)
  , m_inWork(inWork)
  , m_processResultFunc(funcResult)
  , m_mutexResult(mutexResult)
  {}


public:
  static bool Process(size_t numThreads, size_t baseScale, TIndex const & index, TProcessResultFunc funcResult)
  {
    list<TCell> listTasks;
    for (size_t i = 0; i < TCell::TotalCellsOnLevel(baseScale); ++i)
      listTasks.push_back(TCell::FromBitsAndLevel(i, static_cast<int>(baseScale)));

    vector<RegionInCellSplitter> instances;
    vector<thread> threads;
    mutex mutexTasks;
    mutex mutexResult;
    condition_variable condVar;
    size_t inWork = 0;
    for (size_t i = 0; i < numThreads; ++i)
    {
      instances.emplace_back(RegionInCellSplitter(listTasks, mutexTasks, condVar, inWork, funcResult, mutexResult, index));
      threads.emplace_back(thread(instances.back()));
    }

    for (auto & thread : threads)
      thread.join();
    // return true if listTask has no error cells
    return listTasks.empty();
  }

  bool ProcessCell(TCell const & cell)
  {
    // get rect cell
    double minX, minY, maxX, maxY;
    CellIdConverter<MercatorBounds, TCell>::GetCellBounds(cell, minX, minY, maxX, maxY);

    // create rect region
    typedef m2::PointD P;
    PointT arr[] = { D2I(P(minX, minY)), D2I(P(minX, maxY)),
      D2I(P(maxX, maxY)), D2I(P(maxX, minY)) };
    RegionT rectR(arr, arr + ARRAY_SIZE(arr));

    // Do 'and' with all regions and accumulate the result, including bound region.
    // In 'odd' parts we will have an ocean.
    DoDifference doDiff(rectR);
    m_index.ForEachInRect(GetLimitRect(rectR), bind<void>(ref(doDiff), _1));

    // Check if too many points for feature.
    if (cell.Level() < 10 /*m_highLevel*/ && doDiff.GetPointsCount() >= 20000 /*m_maxPoints*/)
      return false;

    {
      // assign feature
      unique_lock<mutex> lock(m_mutexResult);
      m_processResultFunc(cell, doDiff);
    }


    return true;
  }

public:
  void operator()()
  {
    // thread main loop
    for (;;)
    {
      TCell currentCell;
      unique_lock<mutex> lock(m_mutexTasks);
      m_listCondVar.wait(lock, [this]{return (!m_listTasks.empty() || m_inWork == 0);});
      if (m_listTasks.empty() && m_inWork == 0)
        break;

      currentCell = m_listTasks.front();
      m_listTasks.pop_front();
      ++m_inWork;
      lock.unlock();

      bool done = ProcessCell(currentCell);

      lock.lock();
      // return to queue not ready cells
      if (!done)
      {
        for (int8_t i = 0; i < 4; ++i)
          m_listTasks.push_back(currentCell.Child(i));
      }
      --m_inWork;
      m_listCondVar.notify_all();
    }

    // return back cells with error into task queue
    if (!m_errorCell.empty())
    {
      unique_lock<mutex> lock(m_mutexTasks);
      m_listTasks.insert(m_listTasks.end(), m_errorCell.begin(), m_errorCell.end());
    }
  }

};

void CoastlineFeaturesGenerator::GetFeatures(size_t baseLevel, vector<FeatureBuilder1> & features)
{
  size_t maxThreads = thread::thread::hardware_concurrency();
  RegionInCellSplitter::Process(maxThreads, baseLevel, m_tree,
                                [&features, this](RegionInCellSplitter::TCell const & cell, DoDifference & cellData)
  {
    features.emplace_back(FeatureBuilder1());
    FeatureBuilder1 & fb = features.back();
    fb.SetCoastCell(cell.ToInt64(m_highLevel + 1), cell.ToString());

    cellData.AssignGeometry(fb);
    fb.SetArea();
    fb.AddType(m_coastType);

    // should present any geometry
    CHECK_GREATER ( fb.GetPolygonsCount(), 0, () );
    CHECK_GREATER_OR_EQUAL ( fb.GetPointsCount(),  3, () );
  });
}
