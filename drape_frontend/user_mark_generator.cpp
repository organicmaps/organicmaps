#include "drape_frontend/user_mark_generator.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/batcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect_intersect.hpp"

#include "indexer/scales.hpp"

#include <algorithm>

namespace df
{
std::vector<int> const kLineIndexingLevels = {1, 7, 11};

UserMarkGenerator::UserMarkGenerator(TFlushFn const & flushFn)
  : m_flushFn(flushFn)
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

void UserMarkGenerator::SetCreatedUserMarks(drape_ptr<IDCollections> && ids)
{
  if (ids == nullptr)
    return;
  for (auto const & id : ids->m_markIds)
    m_marks[id]->m_justCreated = true;
}

void UserMarkGenerator::SetUserMarks(drape_ptr<UserMarksRenderCollection> && marks)
{
  for (auto & pair : *marks)
  {
    auto it = m_marks.find(pair.first);
    if (it != m_marks.end())
      it->second = std::move(pair.second);
    else
      m_marks.emplace(pair.first, std::move(pair.second));
  }
}

void UserMarkGenerator::SetUserLines(drape_ptr<UserLinesRenderCollection> && lines)
{
  for (auto & pair : *lines)
  {
    auto it = m_lines.find(pair.first);
    if (it != m_lines.end())
      it->second = std::move(pair.second);
    else
      m_lines.emplace(pair.first, std::move(pair.second));
  }
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

  for (auto markId : idCollection.m_markIds)
  {
    UserMarkRenderParams const & params = *m_marks[markId];
    for (int zoomLevel = params.m_minZoom; zoomLevel <= scales::GetUpperScale(); ++zoomLevel)
    {
      TileKey const tileKey = GetTileKeyByPoint(params.m_pivot, zoomLevel);
      auto groupIDs = GetIdCollection(tileKey, groupId);
      groupIDs->m_markIds.push_back(markId);
    }
  }

  for (auto lineId : idCollection.m_lineIds)
  {
    UserLineRenderParams const & params = *m_lines[lineId];

    int const startZoom = GetNearestLineIndexZoom(params.m_minZoom);
    for (int zoomLevel : kLineIndexingLevels)
    {
      if (zoomLevel < startZoom)
        continue;
      // Process spline by segments that are no longer than tile size.
      double const maxLength = MercatorBounds::kRangeX / (1 << (zoomLevel - 1));

      df::ProcessSplineSegmentRects(params.m_spline, maxLength,
                                    [&](m2::RectD const & segmentRect)
      {
        CalcTilesCoverage(segmentRect, zoomLevel, [&](int tileX, int tileY)
        {
          TileKey const tileKey(tileX, tileY, zoomLevel);
          auto groupIDs = GetIdCollection(tileKey, groupId);
          groupIDs->m_lineIds.push_back(lineId);
        });
        return true;
      });
    }
  }

  CleanIndex();
}

ref_ptr<IDCollections> UserMarkGenerator::GetIdCollection(TileKey const & tileKey, kml::MarkGroupId groupId)
{
  ref_ptr<MarksIDGroups> tileGroups;
  auto itTileGroups = m_index.find(tileKey);
  if (itTileGroups == m_index.end())
  {
    auto tileIDGroups = make_unique_dp<MarksIDGroups>();
    tileGroups = make_ref(tileIDGroups);
    m_index.insert(make_pair(tileKey, std::move(tileIDGroups)));
  }
  else
  {
    tileGroups = make_ref(itTileGroups->second);
  }

  ref_ptr<IDCollections> groupIDs;
  auto itGroupIDs = tileGroups->find(groupId);
  if (itGroupIDs == tileGroups->end())
  {
    auto groupMarkIndexes = make_unique_dp<IDCollections>();
    groupIDs = make_ref(groupMarkIndexes);
    tileGroups->insert(make_pair(groupId, std::move(groupMarkIndexes)));
  }
  else
  {
    groupIDs = make_ref(itGroupIDs->second);
  }

  return groupIDs;
}

void UserMarkGenerator::CleanIndex()
{
  for (auto tileIt = m_index.begin(); tileIt != m_index.end();)
  {
    if (tileIt->second->empty())
      tileIt = m_index.erase(tileIt);
    else
      ++tileIt;
  }

  for (auto & tileGroups : m_index)
  {
    for (auto groupIt = tileGroups.second->begin(); groupIt != tileGroups.second->end();)
    {
      if (groupIt->second->IsEmpty())
        groupIt = tileGroups.second->erase(groupIt);
      else
        ++groupIt;
    }
  }
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
  auto const itTile = m_index.find(tileKey);
  if (itTile != m_index.end())
    return make_ref(itTile->second);
  return nullptr;
}

ref_ptr<MarksIDGroups> UserMarkGenerator::GetUserLinesGroups(TileKey const & tileKey)
{
  auto itTile = m_index.end();
  int const lineZoom = GetNearestLineIndexZoom(tileKey.m_zoomLevel);
  CalcTilesCoverage(tileKey.GetGlobalRect(), lineZoom,
                    [this, &itTile, lineZoom](int tileX, int tileY)
  {
    itTile = m_index.find(TileKey(tileX, tileY, lineZoom));
  });
  if (itTile != m_index.end())
    return make_ref(itTile->second);
  return nullptr;
}

void UserMarkGenerator::GenerateUserMarksGeometry(ref_ptr<dp::GraphicsContext> context,
                                                  TileKey const & tileKey,
                                                  ref_ptr<dp::TextureManager> textures)
{
  auto const clippedTileKey =
      TileKey(tileKey.m_x, tileKey.m_y, ClipTileZoomByMaxDataZoom(tileKey.m_zoomLevel));
  auto marksGroups = GetUserMarksGroups(clippedTileKey);
  auto linesGroups = GetUserLinesGroups(clippedTileKey);

  if (marksGroups == nullptr && linesGroups == nullptr)
    return;

  uint32_t constexpr kMaxSize = 65000;
  dp::Batcher batcher(kMaxSize, kMaxSize);
  TUserMarksRenderData renderData;
  {
    dp::SessionGuard guard(context, batcher, [&tileKey, &renderData](dp::RenderState const & state,
                                                                     drape_ptr<dp::RenderBucket> && b)
    {
      renderData.emplace_back(state, std::move(b), tileKey);
    });

    if (marksGroups != nullptr)
      CacheUserMarks(context, tileKey, *marksGroups.get(), textures, batcher);
    if (linesGroups != nullptr)
      CacheUserLines(context, tileKey, *linesGroups.get(), textures, batcher);
  }
  m_flushFn(std::move(renderData));
}

void UserMarkGenerator::CacheUserLines(ref_ptr<dp::GraphicsContext> context,
                                       TileKey const & tileKey, MarksIDGroups const & indexesGroups,
                                       ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher)
{
  for (auto & groupPair : indexesGroups)
  {
    kml::MarkGroupId groupId = groupPair.first;
    if (m_groupsVisibility.find(groupId) == m_groupsVisibility.end())
      continue;

    df::CacheUserLines(context, tileKey, textures, groupPair.second->m_lineIds, m_lines, batcher);
  }
}

void UserMarkGenerator::CacheUserMarks(ref_ptr<dp::GraphicsContext> context,
                                       TileKey const & tileKey, MarksIDGroups const & indexesGroups,
                                       ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher)
{
  for (auto & groupPair : indexesGroups)
  {
    kml::MarkGroupId groupId = groupPair.first;
    if (m_groupsVisibility.find(groupId) == m_groupsVisibility.end())
      continue;
    df::CacheUserMarks(context, tileKey, textures, groupPair.second->m_markIds, m_marks, batcher);
  }
}

int UserMarkGenerator::GetNearestLineIndexZoom(int zoom) const
{
  int nearestZoom = kLineIndexingLevels[0];
  for (size_t i = 1; i < kLineIndexingLevels.size(); ++i)
  {
    if (kLineIndexingLevels[i] <= zoom)
      nearestZoom = kLineIndexingLevels[i];
    else
      break;
  }
  return nearestZoom;
}
}  // namespace df
