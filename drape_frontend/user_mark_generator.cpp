#include "drape_frontend/user_mark_generator.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/utils/vertex_decl.hpp"

namespace df
{
UserMarkGenerator::UserMarkGenerator(TFlushFn const & flushFn)
  : m_flushFn(flushFn)
{
}

void UserMarkGenerator::SetUserMarks(uint32_t groupId, drape_ptr<UserMarksRenderCollection> && marks)
{
  m_marks.insert(make_pair(groupId, std::move(marks)));
  UpdateMarksIndex(groupId);
}

void UserMarkGenerator::SetUserLines(uint32_t groupId, drape_ptr<UserLinesRenderCollection> && lines)
{
  m_lines.insert(make_pair(groupId, std::move(lines)));
  UpdateLinesIndex(groupId);
}

void UserMarkGenerator::UpdateMarksIndex(uint32_t groupId)
{
  for (auto & tileGroups : m_marksIndex)
  {
    auto itGroupIndexes = tileGroups.second->find(groupId);
    if (itGroupIndexes != tileGroups.second->end())
    {
      itGroupIndexes->second->m_markIndexes.clear();
    }
  }

  UserMarksRenderCollection & marks = *m_marks[groupId];
  for (size_t markIndex = 0; markIndex < marks.size(); ++markIndex)
  {
    for (int zoomLevel = 1; zoomLevel <= scales::GetUpperScale(); ++zoomLevel)
    {
      TileKey const tileKey = GetTileKeyByPoint(marks[markIndex].m_pivot, zoomLevel);
      ref_ptr<IndexesCollection> groupIndexes = GetIndexesCollection(tileKey, groupId);
      groupIndexes->m_markIndexes.push_back(static_cast<uint32_t>(markIndex));
    }
  }

  CleanIndex();
}

void UserMarkGenerator::UpdateLinesIndex(uint32_t groupId)
{
  for (auto & tileGroups : m_marksIndex)
  {
    auto itGroupIndexes = tileGroups.second->find(groupId);
    if (itGroupIndexes != tileGroups.second->end())
    {
      itGroupIndexes->second->m_lineIndexes.clear();
    }
  }

  UserLinesRenderCollection & lines = *m_lines[groupId];
  for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
  {
    for (int zoomLevel = 1; zoomLevel <= scales::GetUpperScale(); ++zoomLevel)
    {
      // TODO: Calculate tiles for the line.
      std::set<TileKey> tiles;
      for (size_t i = 0, sz = lines[lineIndex].m_points.size(); i < sz; ++i)
      {
        tiles.insert(GetTileKeyByPoint(lines[lineIndex].m_points[i], zoomLevel));
      }
      for (auto const & tileKey : tiles)
      {
        ref_ptr<IndexesCollection> groupIndexes = GetIndexesCollection(tileKey, groupId);
        groupIndexes->m_lineIndexes.push_back(static_cast<uint32_t>(lineIndex));
      }
    }
  }

  CleanIndex();
}

ref_ptr<IndexesCollection> UserMarkGenerator::GetIndexesCollection(TileKey const & tileKey, GroupID groupId)
{
  ref_ptr<MarkIndexesGroups> tileGroups;
  auto itTileGroups = m_marksIndex.find(tileKey);
  if (itTileGroups == m_marksIndex.end())
  {
    auto tileIndexesGroups = make_unique_dp<MarkIndexesGroups>();
    tileGroups = make_ref(tileIndexesGroups);
    m_marksIndex.insert(make_pair(tileKey, std::move(tileIndexesGroups)));
  }
  else
  {
    tileGroups = make_ref(itTileGroups->second);
  }

  ref_ptr<IndexesCollection> groupIndexes;
  auto itGroupIndexes = tileGroups->find(groupId);
  if (itGroupIndexes == tileGroups->end())
  {
    auto groupMarkIndexes = make_unique_dp<IndexesCollection>();
    groupIndexes = make_ref(groupMarkIndexes);
    tileGroups->insert(make_pair(groupId, std::move(groupMarkIndexes)));
  }
  else
  {
    groupIndexes = make_ref(itGroupIndexes->second);
  }

  return groupIndexes;
}

void UserMarkGenerator::CleanIndex()
{
  for (auto tileIt = m_marksIndex.begin(); tileIt != m_marksIndex.end();)
  {
    if (tileIt->second->empty())
      tileIt = m_marksIndex.erase(tileIt);
    else
      ++tileIt;
  }
}

void UserMarkGenerator::SetGroupVisibility(GroupID groupId, bool isVisible)
{
  m_groupsVisibility[groupId] = isVisible;
}

void UserMarkGenerator::GenerateUserMarksGeometry(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures)
{
  MarksIndex::const_iterator itTile = m_marksIndex.find(tileKey);
  if (itTile == m_marksIndex.end())
    return;

  MarkIndexesGroups & indexesGroups = *itTile->second;
  for (auto & groupPair : indexesGroups)
  {
    GroupID groupId = groupPair.first;
    UserMarksRenderCollection const & renderParams = *m_marks[groupId];
    MarkIndexesCollection const & indexes = groupPair.second->m_markIndexes;

    //if (!m_groupsVisibility[groupId])
    //  continue;

    TUserMarksRenderData renderData;
    UserMarkShape::Draw(tileKey, textures, renderParams, indexes, renderData);

    m_flushFn(groupId, std::move(renderData));
  }
}


}  // namespace df
