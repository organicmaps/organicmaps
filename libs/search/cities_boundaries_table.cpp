#include "search/cities_boundaries_table.hpp"

#include "search/categories_cache.hpp"
#include "search/localities_source.hpp"
#include "search/mwm_context.hpp"

#include "indexer/cities_boundaries_serdes.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/utils.hpp"

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <sstream>

namespace search
{
using namespace indexer;
using namespace std;

// CitiesBoundariesTable::Boundaries ---------------------------------------------------------------
bool CitiesBoundariesTable::Boundaries::HasPoint(m2::PointD const & p) const
{
  return any_of(m_boundaries.begin(), m_boundaries.end(),
                [&](CityBoundary const & b) { return b.HasPoint(p, m_eps); });
}

std::string DebugPrint(CitiesBoundariesTable::Boundaries const & boundaries)
{
  std::ostringstream os;
  os << "Boundaries [";
  os << ::DebugPrint(boundaries.m_boundaries) << ", ";
  os << "eps: " << boundaries.m_eps;
  os << "]";
  return os.str();
}

// CitiesBoundariesTable ---------------------------------------------------------------------------
bool CitiesBoundariesTable::Load()
{
  auto handle = FindWorld(m_dataSource);
  if (!handle.IsAlive())
  {
    LOG(LWARNING, ("Can't find World map file."));
    return false;
  }

  // Skip if table was already loaded from this file.
  if (handle.GetId() == m_mwmId)
    return true;

  MwmContext context(std::move(handle));
  base::Cancellable const cancellable;
  auto const localities = CategoriesCache(LocalitiesSource{}, cancellable).Get(context);

  auto const & cont = context.m_value.m_cont;

  if (!cont.IsExist(CITIES_BOUNDARIES_FILE_TAG))
  {
    LOG(LWARNING, ("No cities boundaries table in the world map."));
    return false;
  }

  vector<vector<CityBoundary>> all;
  double precision;

  try
  {
    auto reader = cont.GetReader(CITIES_BOUNDARIES_FILE_TAG);
    ReaderSource<ReaderPtr<ModelReader>> source(reader);
    CitiesBoundariesSerDes::Deserialize(source, all, precision);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Can't read cities boundaries table from the world map:", e.Msg()));
    return false;
  }

  if (all.size() != localities.PopCount())
  {
    LOG(LERROR, ("Wrong number of boundaries, expected:", localities.PopCount(), "actual:", all.size()));
    return false;
  }

  m_mwmId = context.GetId();
  m_table.clear();
  m_eps = precision;
  size_t idx = 0, notEmpty = 0;
  localities.ForEach([&](uint64_t fid)
  {
    if (!all[idx].empty())
    {
      CHECK(m_table.emplace(base::asserted_cast<uint32_t>(fid), std::move(all[idx])).second, ());
      ++notEmpty;
    }
    ++idx;
  });

  LOG(LDEBUG, ("Localities count =", idx, "; with boundary =", notEmpty));
  return true;
}

bool CitiesBoundariesTable::Get(FeatureID const & fid, Boundaries & bs) const
{
  if (fid.m_mwmId != m_mwmId)
    return false;
  return Get(fid.m_index, bs);
}

bool CitiesBoundariesTable::Get(uint32_t fid, Boundaries & bs) const
{
  auto const it = m_table.find(fid);
  if (it == m_table.end())
    return false;
  bs = Boundaries(it->second, m_eps);
  return true;
}

void GetCityBoundariesInRectForTesting(CitiesBoundariesTable const & table, m2::RectD const & rect,
                                       vector<uint32_t> & featureIds)
{
  featureIds.clear();
  for (auto const & kv : table.m_table)
  {
    for (auto const & cb : kv.second)
    {
      if (rect.IsIntersect(cb.m_bbox.ToRect()))
      {
        featureIds.push_back(kv.first);
        break;
      }
    }
  }
}
}  // namespace search
