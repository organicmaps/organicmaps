#include "drape_frontend/user_mark_generator.hpp"

namespace df
{
UserMarkGenerator::UserMarkGenerator()
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
      itGroupIndexes->second->clear();
  }

  UserMarksRenderCollection & marks = *m_marks[groupId];
  for (size_t markIndex = 0; markIndex < marks.size(); ++markIndex)
  {
    for (size_t zoomLevel = 1; zoomLevel <= scales::GetUpperScale(); ++zoomLevel)
    {
      TileKey const tileKey = GetTileKeyByPoint(marks[markIndex].m_pivot, zoomLevel);

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

      ref_ptr<MarkIndexesCollection> groupIndexes;
      auto itGroupIndexes = tileGroups->find(groupId);
      if (itGroupIndexes == tileGroups->end())
      {
        auto groupMarkIndexes = make_unique_dp<MarkIndexesCollection>();
        groupIndexes = make_ref(groupMarkIndexes);
        tileGroups->insert(make_pair(groupId, std::move(groupMarkIndexes)));
      }
      else
      {
        groupIndexes = make_ref(itGroupIndexes->second);
      }

      groupIndexes->push_back(static_cast<uint32_t>(markIndex));
    }
  }

  for (auto tileIt = m_marksIndex.begin(); tileIt != m_marksIndex.end();)
  {
    if (tileIt->second->empty())
      tileIt = m_marksIndex.erase(tileIt);
    else
      ++tileIt;
  }
}

void UserMarkGenerator::UpdateLinesIndex(uint32_t groupId)
{

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
    MarkIndexesCollection & indexes = *groupPair.second;

    if (!m_groupsVisibility[groupId])
      continue;

    UserMarkBatcherKey batcherKey(tileKey, groupId);
    m_batchersPool->ReserveBatcher(batcherKey);
    ref_ptr<dp::Batcher> batcher = m_batchersPool->GetBatcher(batcherKey);

    dp::TextureManager::SymbolRegion region;

    uint32_t const vertexCount = static_cast<uint32_t>(it->second.size()) * dp::Batcher::VertexPerQuad;
    uint32_t const indicesCount = static_cast<uint32_t>(it->second.size()) * dp::Batcher::IndexPerQuad;
    buffer_vector<UPV, 128> buffer;
    buffer.reserve(vertexCount);

    for (auto const markIndex : indexes)
    {
      UserMarkRenderInfo const & renderInfo = m_marks[markIndex];
      if (!renderInfo.m_isVisible)
        continue;
      textures->GetSymbolRegion(renderInfo.m_symbolName, region);
      m2::RectF const & texRect = region.GetTexRect();
      m2::PointF const pxSize = region.GetPixelSize();
      dp::Anchor const anchor = renderInfo.m_anchor;
      m2::PointD const pt = MapShape::ConvertToLocal(renderInfo.m_pivot, tileKey.GetGlobalRect().Center(), kShapeCoordScalar);
      glsl::vec3 const pos = glsl::vec3(glsl::ToVec2(pt), renderInfo.m_depth);
      bool const runAnim = renderInfo.m_runCreationAnim;

      glsl::vec2 left, right, up, down;
      AlignHorizontal(pxSize.x * 0.5f, anchor, left, right);
      AlignVertical(pxSize.y * 0.5f, anchor, up, down);

      m2::PointD const pixelOffset = pointMark->GetPixelOffset();
      glsl::vec2 const offset(pixelOffset.x, pixelOffset.y);

      buffer.emplace_back(pos, left + down + offset, glsl::ToVec2(texRect.LeftTop()), runAnim);
      buffer.emplace_back(pos, left + up + offset, glsl::ToVec2(texRect.LeftBottom()), runAnim);
      buffer.emplace_back(pos, right + down + offset, glsl::ToVec2(texRect.RightTop()), runAnim);
      buffer.emplace_back(pos, right + up + offset, glsl::ToVec2(texRect.RightBottom()), runAnim);
    }

    dp::GLState state(gpu::BOOKMARK_PROGRAM, dp::GLState::UserMarkLayer);
    state.SetProgram3dIndex(gpu::BOOKMARK_BILLBOARD_PROGRAM);
    state.SetColorTexture(region.GetTexture());
    state.SetTextureFilter(gl_const::GLNearest);

    uint32_t const kMaxSize = 65000;
    dp::Batcher batcher(min(indicesCount, kMaxSize), min(vertexCount, kMaxSize));
    dp::SessionGuard guard(batcher, [&key, &outShapes](dp::GLState const & state,
                                                       drape_ptr<dp::RenderBucket> && b)
    {
      outShapes.emplace_back(UserMarkShape(state, move(b), key));
    });
    dp::AttributeProvider attribProvider(1, static_cast<uint32_t>(buffer.size()));
    attribProvider.InitStream(0, UPV::GetBinding(), make_ref(buffer.data()));
    batcher.InsertListOfStrip(state, make_ref(&attribProvider), dp::Batcher::VertexPerQuad);
  }
}
}  // namespace df
