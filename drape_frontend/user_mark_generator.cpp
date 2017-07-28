#include "drape_frontend/user_mark_generator.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/batcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect_intersect.hpp"

#include "indexer/scales.hpp"

#include <algorithm>

namespace df
{
UserMarkGenerator::UserMarkGenerator(TFlushFn const & flushFn)
  : m_flushFn(flushFn)
{
  ASSERT(m_flushFn != nullptr, ());
}

void UserMarkGenerator::RemoveGroup(MarkGroupID groupId)
{
  m_groupsVisibility.erase(groupId);
  m_groups.erase(groupId);
  UpdateIndex(groupId);
}

void UserMarkGenerator::SetGroup(MarkGroupID groupId, drape_ptr<MarkIDCollection> && ids)
{
  m_groups[groupId] = std::move(ids);
  UpdateIndex(groupId);
}

void UserMarkGenerator::SetRemovedUserMarks(drape_ptr<MarkIDCollection> && ids)
{
  if (ids == nullptr)
    return;
  for (auto const & id : ids->m_marksID)
    m_marks.erase(id);
  for (auto const & id : ids->m_linesID)
    m_lines.erase(id);
}

void UserMarkGenerator::SetCreatedUserMarks(drape_ptr<MarkIDCollection> && ids)
{
  if (ids == nullptr)
    return;
  for (auto const & id : ids->m_marksID)
    m_marks[id].get()->m_justCreated = true;
}

void UserMarkGenerator::SetUserMarks(drape_ptr<UserMarksRenderCollection> && marks)
{
  for (auto & pair : *marks.get())
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
  for (auto & pair : *lines.get())
  {
    auto it = m_lines.find(pair.first);
    if (it != m_lines.end())
      it->second = std::move(pair.second);
    else
      m_lines.emplace(pair.first, std::move(pair.second));
  }
}


void UserMarkGenerator::UpdateIndex(MarkGroupID groupId)
{
  for (auto & tileGroups : m_index)
  {
    auto itGroupIndexes = tileGroups.second->find(groupId);
    if (itGroupIndexes != tileGroups.second->end())
    {
      itGroupIndexes->second->m_marksID.clear();
      itGroupIndexes->second->m_linesID.clear();
    }
  }

  auto const groupIt = m_groups.find(groupId);
  if (groupIt == m_groups.end())
    return;

  MarkIDCollection & idCollection = *groupIt->second.get();

  for (auto markId : idCollection.m_marksID)
  {
    UserMarkRenderParams const & params = *m_marks[markId].get();
    for (int zoomLevel = params.m_minZoom; zoomLevel <= scales::GetUpperScale(); ++zoomLevel)
    {
      TileKey const tileKey = GetTileKeyByPoint(params.m_pivot, zoomLevel);
      ref_ptr<MarkIDCollection> groupIDs = GetIdCollection(tileKey, groupId);
      groupIDs->m_marksID.push_back(static_cast<uint32_t>(markId));
    }
  }

  for (auto lineId : idCollection.m_linesID)
  {
    UserLineRenderParams const & params = *m_lines[lineId].get();

    double const splineFullLength = params.m_spline->GetLength();

    for (int zoomLevel = params.m_minZoom; zoomLevel <= scales::GetUpperScale(); ++zoomLevel)
    {
      double length = 0;
      while (length < splineFullLength)
      {
        // Process spline by segments that no longer than tile size.
        double const range = MercatorBounds::maxX - MercatorBounds::minX;
        double const maxLength = range / (1 << (zoomLevel - 1));

        m2::RectD splineRect;

        auto const itBegin = params.m_spline->GetPoint(length);
        auto itEnd = params.m_spline->GetPoint(length + maxLength);
        if (itEnd.BeginAgain())
        {
          double const lastSegmentLength = params.m_spline->GetLengths().back();
          itEnd = params.m_spline->GetPoint(splineFullLength - lastSegmentLength / 2.0);
          splineRect.Add(params.m_spline->GetPath().back());
        }

        params.m_spline->ForEachNode(itBegin, itEnd, [&splineRect](m2::PointD const & pt)
        {
          splineRect.Add(pt);
        });

        length += maxLength;

        CalcTilesCoverage(splineRect, zoomLevel, [&](int tileX, int tileY)
        {
          TileKey const tileKey(tileX, tileY, zoomLevel);
          ref_ptr<MarkIDCollection> groupIDs = GetIdCollection(tileKey, groupId);
          groupIDs->m_linesID.push_back(static_cast<uint32_t>(lineId));
        });
      }
    }
  }

  CleanIndex();
}

ref_ptr<MarkIDCollection> UserMarkGenerator::GetIdCollection(TileKey const & tileKey, MarkGroupID groupId)
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

  ref_ptr<MarkIDCollection> groupIDs;
  auto itGroupIDs = tileGroups->find(groupId);
  if (itGroupIDs == tileGroups->end())
  {
    auto groupMarkIndexes = make_unique_dp<MarkIDCollection>();
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
      if (groupIt->second->m_marksID.empty() && groupIt->second->m_linesID.empty())
        groupIt = tileGroups.second->erase(groupIt);
      else
        ++groupIt;
    }
  }
}

void UserMarkGenerator::SetGroupVisibility(MarkGroupID groupId, bool isVisible)
{
  if (isVisible)
    m_groupsVisibility.insert(groupId);
  else
    m_groupsVisibility.erase(groupId);
}

void UserMarkGenerator::GenerateUserMarksGeometry(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures)
{
  auto const itTile = m_index.find(TileKey(tileKey.m_x, tileKey.m_y,
                                           std::min(tileKey.m_zoomLevel, scales::GetUpperScale())));
  if (itTile == m_index.end())
    return;

  TUserMarksRenderData renderData;
  {
    uint32_t const kMaxSize = 65000;
    dp::Batcher batcher(kMaxSize, kMaxSize);
    dp::SessionGuard guard(batcher, [&tileKey, &renderData](dp::GLState const & state,
                                                           drape_ptr<dp::RenderBucket> && b)
    {
      renderData.emplace_back(state, std::move(b), tileKey);
    });

    MarksIDGroups & indexesGroups = *itTile->second;
    for (auto & groupPair : indexesGroups)
    {
      MarkGroupID groupId = groupPair.first;

      if (m_groupsVisibility.find(groupId) == m_groupsVisibility.end())
        continue;

      CacheUserMarks(tileKey, textures, groupPair.second->m_marksID, m_marks, batcher);
      CacheUserLines(tileKey, textures, groupPair.second->m_linesID, m_lines, batcher);
    }
  }
  m_flushFn(std::move(renderData));
}
}  // namespace df
