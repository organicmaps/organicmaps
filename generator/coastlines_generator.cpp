#include "generator/coastlines_generator.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/point_coding.hpp"

#include "geometry/region2d/binary_operators.hpp"

#include "base/logging.hpp"

#include <condition_variable>
#include <functional>
#include <thread>

namespace coastlines_generator
{
using RegionT = m2::RegionI;
using PointT = m2::PointI;
using RectT = m2::RectI;

m2::RectD GetLimitRect(RegionT const & rgn)
{
  RectT const r = rgn.GetRect();
  return m2::RectD(r.minX(), r.minY(), r.maxX(), r.maxY());
}

PointT D2I(m2::PointD const & p)
{
  m2::PointU const pu = PointDToPointU(p, kPointCoordBits);
  return PointT(static_cast<int32_t>(pu.x), static_cast<int32_t>(pu.y));
}

m2::RegionI CreateRegionI(std::vector<m2::PointD> const & poly)
{
  CHECK_GREATER(poly.size(), 2, ());
  CHECK(poly.front().EqualDxDy(poly.back(), 1.0E-12), (poly.front(), poly.back()));

  m2::RegionI rgn;
  for (auto it = poly.begin() + 1; it != poly.end(); ++it)
    rgn.AddPoint(D2I(*it));
  return rgn;
}

class DoAddToTree : public FeatureEmitterIFace
{
  CoastlineFeaturesGenerator & m_rMain;
  size_t m_notMergedCoastsCount;
  size_t m_totalNotMergedCoastsPoints;

public:
  explicit DoAddToTree(CoastlineFeaturesGenerator & rMain)
    : m_rMain(rMain)
    , m_notMergedCoastsCount(0)
    , m_totalNotMergedCoastsPoints(0)
  {}

  virtual void operator()(feature::FeatureBuilder const & fb)
  {
    if (fb.IsGeometryClosed())
      m_rMain.AddRegionToTree(fb);
    else
    {
      base::GeoObjectId const firstWay = fb.GetFirstOsmId();
      base::GeoObjectId const lastWay = fb.GetLastOsmId();
      if (firstWay == lastWay)
        LOG(LINFO, ("Not merged coastline, way", firstWay.GetSerialId(), "(", fb.GetPointsCount(), "points)"));
      else
        LOG(LINFO, ("Not merged coastline, ways", firstWay.GetSerialId(), "to", lastWay.GetSerialId(), "(",
                    fb.GetPointsCount(), "points)"));
      ++m_notMergedCoastsCount;
      m_totalNotMergedCoastsPoints += fb.GetPointsCount();
    }
  }

  bool HasNotMergedCoasts() const { return m_notMergedCoastsCount != 0; }

  size_t GetNotMergedCoastsCount() const { return m_notMergedCoastsCount; }

  size_t GetNotMergedCoastsPoints() const { return m_totalNotMergedCoastsPoints; }
};

class DoDifference
{
  RectT m_src;
  std::vector<RegionT> m_res;

public:
  explicit DoDifference(RegionT const & rgn)
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

  size_t GetPointsCount() const
  {
    size_t count = 0;
    for (size_t i = 0; i < m_res.size(); ++i)
      count += m_res[i].GetPointsCount();
    return count;
  }

  void AssignGeometry(feature::FeatureBuilder & fb)
  {
    for (size_t i = 0; i < m_res.size(); ++i)
    {
      std::vector<m2::PointD> points;
      points.reserve(m_res[i].Size() + 1);

      m_res[i].ForEachPoint([&points](PointT const & p)
      {
        points.push_back(
            PointUToPointD(m2::PointU(static_cast<uint32_t>(p.x), static_cast<uint32_t>(p.y)), kPointCoordBits));
      });

      fb.AddPolygon(std::move(points));
    }
  }
};
}  // namespace coastlines_generator

CoastlineFeaturesGenerator::CoastlineFeaturesGenerator() : m_merger(kPointCoordBits) {}

void CoastlineFeaturesGenerator::AddRegionToTree(feature::FeatureBuilder const & fb)
{
  ASSERT(fb.IsGeometryClosed(), ());

  fb.ForEachPolygon([&](auto const & polygon)
  {
    if (polygon.empty())
      return;

    using namespace coastlines_generator;
    RegionT rgn = CreateRegionI(polygon);
    auto const limitRect = GetLimitRect(rgn);

    m_tree.Add(std::move(rgn), limitRect);
  });
}

void CoastlineFeaturesGenerator::Process(feature::FeatureBuilder const & fb)
{
  if (fb.IsGeometryClosed())
    AddRegionToTree(fb);
  else
    m_merger(fb);
}

bool CoastlineFeaturesGenerator::Finish()
{
  coastlines_generator::DoAddToTree doAdd(*this);
  m_merger.DoMerge(doAdd);

  if (doAdd.HasNotMergedCoasts())
  {
    LOG(LINFO, ("Total not merged coasts:", doAdd.GetNotMergedCoastsCount()));
    LOG(LINFO, ("Total points in not merged coasts:", doAdd.GetNotMergedCoastsPoints()));
    return false;
  }

  return true;
}

class RegionInCellSplitter final
{
public:
  using TCell = RectId;
  using TIndex = m4::Tree<m2::RegionI>;
  using TProcessResultFunc = std::function<void(TCell const &, coastlines_generator::DoDifference &)>;

  static int constexpr kStartLevel = 4;
  static int constexpr kHighLevel = 10;
  static int constexpr kMaxPoints = 20000;

protected:
  struct Context
  {
    std::mutex mutexTasks;
    std::list<TCell> listTasks;
    std::condition_variable listCondVar;
    size_t inWork = 0;
    TProcessResultFunc processResultFunc;
  };

  Context & m_ctx;
  TIndex const & m_index;

  RegionInCellSplitter(Context & ctx, TIndex const & index) : m_ctx(ctx), m_index(index) {}

public:
  static bool Process(size_t numThreads, size_t baseScale, TIndex const & index, TProcessResultFunc funcResult)
  {
    /// @todo Replace with base::ComputationalThreadPool

    Context ctx;

    for (size_t i = 0; i < TCell::TotalCellsOnLevel(baseScale); ++i)
      ctx.listTasks.push_back(TCell::FromBitsAndLevel(i, static_cast<int>(baseScale)));

    ctx.processResultFunc = funcResult;

    std::vector<RegionInCellSplitter> instances;
    std::vector<std::thread> threads;
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
    CellIdConverter<mercator::Bounds, TCell>::GetCellBounds(cell, minX, minY, maxX, maxY);

    using namespace coastlines_generator;
    // create rect region
    PointT arr[] = {D2I(m2::PointD(minX, minY)), D2I(m2::PointD(minX, maxY)), D2I(m2::PointD(maxX, maxY)),
                    D2I(m2::PointD(maxX, minY))};
    RegionT rectR(arr, arr + ARRAY_SIZE(arr));

    // Do 'and' with all regions and accumulate the result, including bound region.
    // In 'odd' parts we will have an ocean.
    DoDifference doDiff(rectR);
    m_index.ForEachInRect(GetLimitRect(rectR), std::bind<void>(std::ref(doDiff), std::placeholders::_1));

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
      std::unique_lock lock(m_ctx.mutexTasks);
      m_ctx.listCondVar.wait(lock, [&] { return (!m_ctx.listTasks.empty() || m_ctx.inWork == 0); });
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
        for (int8_t i = 0; i < TCell::MAX_CHILDREN; ++i)
          m_ctx.listTasks.push_back(currentCell.Child(i));
      --m_ctx.inWork;
      m_ctx.listCondVar.notify_all();
    }
  }
};

std::vector<feature::FeatureBuilder> CoastlineFeaturesGenerator::GetFeatures(size_t maxThreads)
{
  uint32_t const coastType = ftypes::IsCoastlineChecker::Instance().GetCoastlineType();

  std::vector<feature::FeatureBuilder> features;
  std::mutex featuresMutex;

  RegionInCellSplitter::Process(
      maxThreads, RegionInCellSplitter::kStartLevel, m_tree,
      [&](RegionInCellSplitter::TCell const & cell, coastlines_generator::DoDifference & cellData)
  {
    feature::FeatureBuilder fb;
    fb.SetCoastCell(cell.ToInt64(RegionInCellSplitter::kHighLevel + 1));

    cellData.AssignGeometry(fb);
    fb.SetArea();
    fb.AddType(coastType);

    // Should represent non-empty geometry
    CHECK_GREATER(fb.GetPolygonsCount(), 0, ());
    CHECK_GREATER_OR_EQUAL(fb.GetPointsCount(), 3, ());

    std::lock_guard lock(featuresMutex);
    features.emplace_back(std::move(fb));
  });

  return features;
}
