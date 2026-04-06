#include "drape_frontend/user_mark_generator.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/batcher.hpp"

#include "indexer/scales.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"

namespace df
{
std::array<int, 3> const kLineIndexingLevels = {1, 7, 11};

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
    for (int zoomLevel = params.m_minZoom; zoomLevel <= scales::GetUpperScale(); ++zoomLevel)
    {
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

    int const startZoom = GetNearestLineIndexZoom(params.m_minZoom);
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

ref_ptr<MarksIDGroups> UserMarkGenerator::GetUserMarksGroups(TileKey const & tileKey)
{
  // Wrap to canonical tile key so extended tiles (past the antimeridian)
  // find marks indexed under canonical coordinates.
  auto const itTile = m_index.find(tileKey.GetCanonicalTileKey());
  if (itTile != m_index.end())
    return make_ref(itTile->second);
  return nullptr;
}

ref_ptr<MarksIDGroups> UserMarkGenerator::GetUserLinesGroups(TileKey const & tileKey)
{
  int const lineZoom = GetNearestLineIndexZoom(tileKey.m_zoomLevel);

  // Use wrapped data rect so extended tiles (past the antimeridian)
  // produce canonical tile coordinates that match the index.
  auto tileRect = tileKey.GetWrappedDataRect();

  // We know for sure that child-parent boundary tiles should have _equal_ boundary coordinates.
  // Reduce input tile rect to avoid unnecessary overlapping with neibour tiles because of calculation errors.
  /// @todo Better to use child-parent tile arithmetics here instead of raw rect covering.
  tileRect.Inflate(-kMwmPointAccuracy, -kMwmPointAccuracy);

  auto const cov = CalcTilesCoverage(tileKey.GetWrappedDataRect(), lineZoom, nullptr);
  // Should be the _one_ covering tile since with selected lineZoom <= tileKey.m_zoomLevel above.
  /// @todo I see visible lags now on Desktop app. Better strategy is:
  /// - make covering with child index tiles (e.g. when tileKey.m_zoomLevel < lineZoom)
  /// - return vector<ref_ptr<MarksIDGroups>>
  /// - use visited set when processing TrackId
  ASSERT(cov.IsOneTile(), ());

  auto itTile = m_index.find(TileKey(cov.m_minTileX, cov.m_minTileY, lineZoom));
  return itTile != m_index.end() ? make_ref(itTile->second) : nullptr;
}

void UserMarkGenerator::GenerateUserMarksGeometry(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                                  ref_ptr<dp::TextureManager> textures)
{
  auto const clippedTileKey = TileKey(tileKey.m_x, tileKey.m_y, ClipTileZoomByMaxDataZoom(tileKey.m_zoomLevel));
  auto marksGroups = GetUserMarksGroups(clippedTileKey);
  auto linesGroups = GetUserLinesGroups(clippedTileKey);

  if (marksGroups == nullptr && linesGroups == nullptr)
    return;

  uint32_t constexpr kMaxSize = 65000;
  dp::Batcher batcher(kMaxSize, kMaxSize);
  batcher.SetBatcherHash(tileKey.GetHashValue(BatcherBucket::UserMark));
  TUserMarksRenderData renderData;
  {
    dp::SessionGuard guard(context, batcher,
                           [&tileKey, &renderData](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    { renderData.emplace_back(state, std::move(b), tileKey); });

    if (marksGroups != nullptr)
      CacheUserMarks(context, tileKey, *marksGroups.get(), textures, batcher);
    if (linesGroups != nullptr)
      CacheUserLines(context, tileKey, *linesGroups.get(), textures, batcher);
  }
  m_flushFn(std::move(renderData));
}

void UserMarkGenerator::CacheUserLines(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                       MarksIDGroups const & indexesGroups, ref_ptr<dp::TextureManager> textures,
                                       dp::Batcher & batcher) const
{
  for (auto const & gp : indexesGroups)
    if (m_groupsVisibility.find(gp.first) != m_groupsVisibility.end())
      df::CacheUserLines(context, tileKey, textures, gp.second->m_lineIds, m_lines, batcher);
}

void UserMarkGenerator::CacheUserMarks(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                       MarksIDGroups const & indexesGroups, ref_ptr<dp::TextureManager> textures,
                                       dp::Batcher & batcher) const
{
  for (auto const & gp : indexesGroups)
    if (m_groupsVisibility.find(gp.first) != m_groupsVisibility.end())
      df::CacheUserMarks(context, tileKey, textures, gp.second->m_markIds, m_marks, batcher);
}

int UserMarkGenerator::GetNearestLineIndexZoom(int zoom) const
{
  int nearestZoom = kLineIndexingLevels[0];
  for (size_t i = 1; i < kLineIndexingLevels.size(); ++i)
    if (kLineIndexingLevels[i] <= zoom)
      nearestZoom = kLineIndexingLevels[i];
    else
      break;
  return nearestZoom;
}
}  // namespace df
