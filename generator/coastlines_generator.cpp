#include "generator/coastlines_generator.hpp"

#include "generator/feature_builder.hpp"

#include "coding/point_coding.hpp"

#include "geometry/region2d/binary_operators.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include <condition_variable>
#include <functional>
#include <thread>
#include <utility>

using namespace std;

using RegionT = m2::RegionI;
using PointT = m2::PointI;
using RectT = m2::RectI;

CoastlineFeaturesGenerator::CoastlineFeaturesGenerator(uint32_t coastType)
  : m_merger(kPointCoordBits), m_coastType(coastType)
{
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
    m2::PointU const pu = PointDToPointU(p, kPointCoordBits);
    return PointT(static_cast<int32_t>(pu.x), static_cast<int32_t>(pu.y));
  }

  template <class Tree> class DoCreateRegion
  {
    Tree & m_tree;

    RegionT m_rgn;
    m2::PointD m_pt;
    bool m_exist;

  public:
    template <typename T>
    DoCreateRegion(T && tree) : m_tree(std::forward<T>(tree)), m_exist(false) {}

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
}  // namespace

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
        base::GeoObjectId const firstWay = fb.GetFirstOsmId();
        base::GeoObjectId const lastWay = fb.GetLastOsmId();
        if (firstWay == lastWay)
          LOG(LINFO, ("Not merged coastline, way", firstWay.GetSerialId(), "(", fb.GetPointsCount(),
                      "points)"));
        else
          LOG(LINFO, ("Not merged coastline, ways", firstWay.GetSerialId(), "to",
                      lastWay.GetSerialId(), "(", fb.GetPointsCount(), "points)"));
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

  void operator()(RegionT const & r)
  {
    // if r is fully inside source rect region,
    // put it to the result vector without any intersection
    if (m_src.IsRectInside(r.GetRect()))
      m_res.push_back(r);
    else
      m2::IntersectRegions(m_res.front(), r, m_res);
  }

  void operator()(PointT const & p)
  {
    m_points.push_back(PointUToPointD(
        m2::PointU(static_cast<uint32_t>(p.x), static_cast<uint32_t>(p.y)), kPointCoordBits));
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
  using TCell = RectId;
  using TIndex = m4::Tree<m2::RegionI>;
  using TProcessResultFunc = function<void(TCell const &, DoDifference &)>;

  static int constexpr kStartLevel = 4;
  static int constexpr kHighLevel = 10;
  static int constexpr kMaxPoints = 20000;

protected:
  struct Context
  {
    mutex mutexTasks;
    list<TCell> listTasks;
    condition_variable listCondVar;
    size_t inWork = 0;
    TProcessResultFunc processResultFunc;
  };

  Context & m_ctx;
  TIndex const & m_index;

  RegionInCellSplitter(Context & ctx,TIndex const & index)
  : m_ctx(ctx), m_index(index)
  {}

public:
  static bool Process(size_t numThreads, size_t baseScale, TIndex const & index,
                      TProcessResultFunc funcResult)
  {
    Context ctx;

    for (size_t i = 0; i < TCell::TotalCellsOnLevel(baseScale); ++i)
      ctx.listTasks.push_back(TCell::FromBitsAndLevel(i, static_cast<int>(baseScale)));

    ctx.processResultFunc = funcResult;

    vector<RegionInCellSplitter> instances;
    vector<thread> threads;
    for (size_t i = 0; i < numThreads; ++i)
    {
      instances.emplace_back(RegionInCellSplitter(ctx, index));
      threads.emplace_back(instances.back());
    }

    for (auto & thread : threads)
      thread.join();

    // return true if listTask has no error cells
    return ctx.listTasks.empty();
  }

  bool ProcessCell(TCell const & cell)
  {
    // get rect cell
    double minX, minY, maxX, maxY;
    CellIdConverter<MercatorBounds, TCell>::GetCellBounds(cell, minX, minY, maxX, maxY);

    // create rect region
    PointT arr[] = {D2I(m2::PointD(minX, minY)), D2I(m2::PointD(minX, maxY)),
                    D2I(m2::PointD(maxX, maxY)), D2I(m2::PointD(maxX, minY))};
    RegionT rectR(arr, arr + ARRAY_SIZE(arr));

    // Do 'and' with all regions and accumulate the result, including bound region.
    // In 'odd' parts we will have an ocean.
    DoDifference doDiff(rectR);
    m_index.ForEachInRect(GetLimitRect(rectR), bind<void>(ref(doDiff), placeholders::_1));

    // Check if too many points for feature.
    if (cell.Level() < kHighLevel && doDiff.GetPointsCount() >= kMaxPoints)
      return false;

    m_ctx.processResultFunc(cell, doDiff);
    return true;
  }

  void operator()()
  {
    // thread main loop
    while (true)
    {
      unique_lock<mutex> lock(m_ctx.mutexTasks);
      m_ctx.listCondVar.wait(lock, [this]{return (!m_ctx.listTasks.empty() || m_ctx.inWork == 0);});
      if (m_ctx.listTasks.empty())
        break;

      TCell currentCell = m_ctx.listTasks.front();
      m_ctx.listTasks.pop_front();
      ++m_ctx.inWork;
      lock.unlock();

      bool const done = ProcessCell(currentCell);

      lock.lock();
      // return to queue not ready cells
      if (!done)
      {
        for (int8_t i = 0; i < TCell::MAX_CHILDREN; ++i)
          m_ctx.listTasks.push_back(currentCell.Child(i));
      }
      --m_ctx.inWork;
      m_ctx.listCondVar.notify_all();
    }
  }
};

void CoastlineFeaturesGenerator::GetFeatures(vector<FeatureBuilder1> & features)
{
  size_t const maxThreads = thread::hardware_concurrency();
  CHECK_GREATER(maxThreads, 0, ("Not supported platform"));

  mutex featuresMutex;
  RegionInCellSplitter::Process(
      maxThreads, RegionInCellSplitter::kStartLevel, m_tree,
      [&features, &featuresMutex, this](RegionInCellSplitter::TCell const & cell, DoDifference & cellData)
      {
        FeatureBuilder1 fb;
        fb.SetCoastCell(cell.ToInt64(RegionInCellSplitter::kHighLevel + 1));

        cellData.AssignGeometry(fb);
        fb.SetArea();
        fb.AddType(m_coastType);

        // Should represent non-empty geometry
        CHECK_GREATER(fb.GetPolygonsCount(), 0, ());
        CHECK_GREATER_OR_EQUAL(fb.GetPointsCount(), 3, ());

        // save result
        lock_guard<mutex> lock(featuresMutex);
        features.emplace_back(move(fb));
      });
}
