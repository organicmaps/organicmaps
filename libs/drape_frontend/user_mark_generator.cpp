#include "drape_frontend/user_mark_generator.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/batcher.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"

namespace df
{
namespace
{
std::array<int, 3> const kLineIndexingLevels = {4, 8, 12};
/// @todo Test as-is for now, probably worth to add additional 16 scale.
std::array<int, 3> const kMarkIndexingLevels = {4, 8, 12};

int GetNearestIndexZoom(auto const & levels, int zoom)
{
  for (auto z : levels)
    if (zoom <= z)
      return z;
  return levels.back();
}
}  // namespace

UserMarkGenerator::UserMarkGenerator(TFlushFn const & flushFn) : m_flushFn(flushFn)
{
  ASSERT(m_flushFn, ());
}

void UserMarkGenerator::RemoveGroup(kml::MarkGroupId groupId)
{
  m_groupsVisibility.erase(groupId);
  m_groups.erase(groupId);
  UpdateIndex(groupId);
}

void UserMarkGenerator::SetGroup(kml::MarkGroupId groupId, drape_ptr<IDCollections> && ids)
{
  m_groups[groupId] = std::move(ids);
  UpdateIndex(groupId);
}

void UserMarkGenerator::SetRemovedUserMarks(drape_ptr<IDCollections> && ids)
{
  if (ids == nullptr)
    return;
  for (auto const & id : ids->m_markIds)
    m_marks.erase(id);
  for (auto const & id : ids->m_lineIds)
    m_lines.erase(id);
}

void UserMarkGenerator::SetJustCreatedUserMarks(drape_ptr<IDCollections> && ids)
{
  if (ids == nullptr)
    return;
  for (auto const & id : ids->m_markIds)
    m_marks[id]->m_justCreated = true;
}

void UserMarkGenerator::SetUserMarks(drape_ptr<UserMarksRenderCollection> && marks)
{
  for (auto & pair : *marks)
    m_marks.insert_or_assign(pair.first, std::move(pair.second));
}

void UserMarkGenerator::SetUserLines(drape_ptr<UserLinesRenderCollection> && lines)
{
  for (auto & pair : *lines)
    m_lines.insert_or_assign(pair.first, std::move(pair.second));
}

void UserMarkGenerator::UpdateIndex(kml::MarkGroupId groupId)
{
  for (auto & tileGroups : m_index)
  {
    auto itGroupIndexes = tileGroups.second->find(groupId);
    if (itGroupIndexes != tileGroups.second->end())
    {
      itGroupIndexes->second->m_markIds.clear();
      itGroupIndexes->second->m_lineIds.clear();
    }
  }

  auto const groupIt = m_groups.find(groupId);
  if (groupIt == m_groups.end())
    return;

  IDCollections & idCollection = *groupIt->second;

  for (auto const & markId : idCollection.m_markIds)
  {
    UserMarkRenderParams const & params = *m_marks[markId];
    int const startZoom = GetNearestIndexZoom(kMarkIndexingLevels, params.m_minZoom);
    for (int zoomLevel : kMarkIndexingLevels)
    {
      if (zoomLevel < startZoom)
        continue;

      TileKey const tileKey = GetTileKeyByPoint(params.m_pivot, zoomLevel);
      auto groupIDs = GetIdCollection(tileKey, groupId);
      groupIDs->m_markIds.push_back(markId);
    }
  }

  for (auto const & lineId : idCollection.m_lineIds)
  {
    // Collect unique intersected tiles.
    std::set<TileKey> tiles;

    UserLineRenderParams const & params = *m_lines[lineId];

    int const startZoom = GetNearestIndexZoom(kLineIndexingLevels, params.m_minZoom);
    for (int zoomLevel : kLineIndexingLevels)
    {
      if (zoomLevel < startZoom)
        continue;

      for (auto const & spline : params.m_splines)
      {
        auto const cov = CalcTilesCoverage(spline->GetRect(), zoomLevel, nullptr);
        if (cov.IsOneTile())  // Most likely path
          tiles.emplace(cov.m_minTileX, cov.m_minTileY, zoomLevel);
        else if (cov.GetTilesCount() <= 16)
        {
          cov.ForEach([&](int tileX, int tileY)
          {
            TileKey tileKey(tileX, tileY, zoomLevel);
            if (!tiles.contains(tileKey) && m2::IsRealIntersect(tileKey.GetWrappedDataRect(), *spline))
              tiles.insert(tileKey);
          });
        }
        else  // Fallback to the approx covering, avoiding O(tilesCount * segsCount) complexity.
        {
          double const maxLength = mercator::Bounds::kRangeX / (1 << (zoomLevel - 1));
          m2::ForEachSection(*spline, maxLength, [&](m2::PointD const & p1, m2::PointD const & p2)
          {
            CalcTilesCoverage(m2::RectD(p1, p2), zoomLevel,
                              [&](int tileX, int tileY) { tiles.emplace(tileX, tileY, zoomLevel); });
          });
        }
      }
    }

    for (auto const & tileKey : tiles)
    {
      auto groupIDs = GetIdCollection(tileKey, groupId);
      groupIDs->m_lineIds.push_back(lineId);
    }
  }

  CleanIndex();
}

ref_ptr<IDCollections> UserMarkGenerator::GetIdCollection(TileKey const & tileKey, kml::MarkGroupId groupId)
{
  auto [itTileGroups, inserted1] = m_index.emplace(tileKey, nullptr);
  if (inserted1)
    itTileGroups->second = make_unique_dp<MarksIDGroups>();

  auto [itGroupIDs, inserted2] = itTileGroups->second->emplace(groupId, nullptr);
  if (inserted2)
    itGroupIDs->second = make_unique_dp<IDCollections>();

  return make_ref(itGroupIDs->second);
}

void UserMarkGenerator::CleanIndex()
{
  for (auto & tileGroups : m_index)
  {
    auto & groups = tileGroups.second;
    for (auto groupIt = groups->begin(); groupIt != groups->end();)
      if (groupIt->second->IsEmpty())
        groupIt = groups->erase(groupIt);
      else
        ++groupIt;
  }

  for (auto tileIt = m_index.begin(); tileIt != m_index.end();)
    if (tileIt->second->empty())
      tileIt = m_index.erase(tileIt);
    else
      ++tileIt;
}

void UserMarkGenerator::SetGroupVisibility(kml::MarkGroupId groupId, bool isVisible)
{
  if (isVisible)
    m_groupsVisibility.insert(groupId);
  else
    m_groupsVisibility.erase(groupId);
}

template <class SourceT, class LevelsT>
SourceT UserMarkGenerator::GetIndexSource(TileKey const & tileKey, LevelsT const & levels) const
{
  int const indexZoom = GetNearestIndexZoom(levels, tileKey.m_zoomLevel);

  // Use wrapped data rect so extended tiles (past the antimeridian)
  // produce canonical tile coordinates that match the index.
  auto tileRect = tileKey.GetWrappedDataRect();

  // Reduce input tile rect to avoid unnecessary overlapping with neighbour tiles because of calculation errors.
  /// @todo Better to use child-parent tile arithmetics here instead of raw rect covering.
  tileRect.Inflate(-kMwmPointAccuracy, -kMwmPointAccuracy);

  SourceT source(&m_groupsVisibility);

  auto const cov = CalcTilesCoverage(tileRect, indexZoom, nullptr);
  cov.ForEach([&](int tileX, int tileY)
  {
    auto itTile = m_index.find(TileKey(tileX, tileY, indexZoom));
    if (itTile != m_index.end())
      source.AddGroup(make_ref(itTile->second));
  });

  return source;
}

void UserMarkGenerator::GenerateUserMarksGeometry(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                                  ref_ptr<dp::TextureManager> textures)
{
  auto const clippedTileKey = TileKey(tileKey.m_x, tileKey.m_y, ClipTileZoomByMaxDataZoom(tileKey.m_zoomLevel));

  auto marksSource = GetIndexSource<MarksSource>(clippedTileKey, kMarkIndexingLevels);
  auto linesSource = GetIndexSource<TracksSource>(clippedTileKey, kLineIndexingLevels);

  if (marksSource.IsEmpty() && linesSource.IsEmpty())
    return;

  uint32_t constexpr kMaxSize = 65000;
  dp::Batcher batcher(kMaxSize, kMaxSize);
  batcher.SetBatcherHash(tileKey.GetHashValue(BatcherBucket::UserMark));
  TUserMarksRenderData renderData;
  {
    dp::SessionGuard guard(context, batcher,
                           [&tileKey, &renderData](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    { renderData.emplace_back(state, std::move(b), tileKey); });

    if (!marksSource.IsEmpty())
      df::CacheUserMarks(context, tileKey, textures, marksSource, m_marks, batcher);
    if (!linesSource.IsEmpty())
      df::CacheUserLines(context, tileKey, textures, linesSource, m_lines, batcher);
  }

  m_flushFn(std::move(renderData));
}

}  // namespace df
