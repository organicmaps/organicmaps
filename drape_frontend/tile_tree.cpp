#include "tile_tree.hpp"

#include "../std/algorithm.hpp"
#include "../std/utility.hpp"

namespace df
{

TileTree::TileTree()
  : m_root(new Node())
{
}

TileTree::~TileTree()
{
  m_root.reset();
  ResetHandlers();
}

void TileTree::SetHandlers(TRenderGroupHandler const & addRenderGroup,
                           TRenderGroupHandler const & deferRenderGroup,
                           TTileHandler const & addDeferredTile,
                           TTileHandler const & removeTile)
{
  m_addRenderGroup = addRenderGroup;
  m_deferRenderGroup = deferRenderGroup;
  m_addDeferredTile = addDeferredTile;
  m_removeTile = removeTile;
}

void TileTree::ResetHandlers()
{
  m_addRenderGroup = nullptr;
  m_deferRenderGroup = nullptr;
  m_addDeferredTile = nullptr;
  m_removeTile = nullptr;
}

void TileTree::Clear()
{
  m_root.reset(new Node());
}

void TileTree::BeginRequesting(int const zoomLevel)
{
  AbortTiles(m_root, zoomLevel);
}

void TileTree::RequestTile(TileKey const & tileKey)
{
  InsertToNode(m_root, tileKey);
}

void TileTree::EndRequesting()
{
  SimplifyTree();
}

void TileTree::GetTilesCollection(TTilesCollection & tiles, int const zoomLevel)
{
  FillTilesCollection(m_root, tiles, zoomLevel);
}

void TileTree::ClipByRect(m2::RectD const & rect)
{
  ClipNode(m_root, rect);
  CheckDeferredTiles(m_root);
  SimplifyTree();
}

bool TileTree::ProcessTile(TileKey const & tileKey, dp::GLState const & state,
                           dp::MasterPointer<dp::RenderBucket> & bucket)
{
  bool const result = ProcessNode(m_root, tileKey, state, bucket);
  if (result)
      SimplifyTree();

  return result;
}

void TileTree::FinishTile(TileKey const & tileKey)
{
  if (FinishNode(m_root, tileKey))
  {
    CheckDeferredTiles(m_root);
    SimplifyTree();
  }
}

void TileTree::InsertToNode(TNodePtr const & node, TileKey const & tileKey)
{
  // here we try to insert new tile to the node. The tree is built in such way that
  // child nodes always have got the same zoom level

  // insert to empty node
  if (node->m_children.empty())
  {
    node->m_children.emplace_back(TNodePtr(new Node(tileKey, TileStatus::Requested)));
    return;
  }

  int const childrenZoomLevel = node->m_children.front()->m_tileKey.m_zoomLevel;
  if (tileKey.m_zoomLevel > childrenZoomLevel)
  {
    // here zoom level of node's children less than new tile's zoom level
    // so we are looking for node to insert new tile recursively

    // looking for parent node
    auto parentNodeIt = find_if(node->m_children.begin(), node->m_children.end(), [&tileKey](TNodePtr const & n)
    {
      return IsTileBelow(n->m_tileKey, tileKey);
    });

    // insert to parent node
    if (parentNodeIt == node->m_children.end())
    {
      TileKey parentTileKey = GetParentTile(tileKey, childrenZoomLevel);
      node->m_children.emplace_back(TNodePtr(new Node(parentTileKey, TileStatus::Unknown)));
      InsertToNode(node->m_children.back(), tileKey);
    }
    else
      InsertToNode(*parentNodeIt, tileKey);
  }
  else if (tileKey.m_zoomLevel < childrenZoomLevel)
  {
    // here zoom level of node's children more than new tile's zoom level
    // so we paste new tile and redistribute children of current node
    // between new tile and his siblings

    list<TNodePtr> newChildren;
    newChildren.emplace_back(new Node(tileKey, TileStatus::Requested));
    for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    {
      // looking for parent node
      TileKey parentTileKey = GetParentTile((*it)->m_tileKey, tileKey.m_zoomLevel);
      auto parentNodeIt = find_if(newChildren.begin(), newChildren.end(), [&parentTileKey](TNodePtr const & n)
      {
        return n->m_tileKey == parentTileKey;
      });

      // insert to parent node
      if (parentNodeIt == newChildren.end())
      {
        newChildren.emplace_back(TNodePtr(new Node(parentTileKey, TileStatus::Unknown)));
        newChildren.back()->m_children.emplace_back(move(*it));
      }
      else
        (*parentNodeIt)->m_children.emplace_back(move(*it));
    }
    node->m_children.swap(newChildren);
  }
  else
  {
    // here zoom level of node's children equals to new tile's zoom level
    // so we insert new tile if we haven't got one

    auto it = find_if(node->m_children.begin(), node->m_children.end(), [&tileKey](TNodePtr const & n)
    {
      return n->m_tileKey == tileKey;
    });
    if (it != node->m_children.end())
    {
      if ((*it)->m_tileStatus == TileStatus::Unknown)
      {
        (*it)->m_tileStatus = TileStatus::Requested;
        (*it)->m_isRemoved = false;
      }
    }
    else
      node->m_children.emplace_back(TNodePtr(new Node(tileKey, TileStatus::Requested)));
  }
}

void TileTree::AbortTiles(TNodePtr const & node, int const zoomLevel)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if ((*it)->m_tileKey.m_zoomLevel != zoomLevel)
    {
      if ((*it)->m_tileStatus == TileStatus::Requested)
        (*it)->m_tileStatus = TileStatus::Unknown;
      else if ((*it)->m_tileStatus == TileStatus::Deferred)
        RemoveTile(*it);
    }

    AbortTiles(*it, zoomLevel);
  }
}

void TileTree::FillTilesCollection(TNodePtr const & node, TTilesCollection & tiles, int const zoomLevel)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if ((*it)->m_tileStatus != TileStatus::Unknown && (*it)->m_tileKey.m_zoomLevel == zoomLevel)
      tiles.insert((*it)->m_tileKey);

    FillTilesCollection(*it, tiles, zoomLevel);
  }
}

void TileTree::ClipNode(TNodePtr const & node, m2::RectD const & rect)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end();)
  {
    m2::RectD const tileRect = (*it)->m_tileKey.GetGlobalRect();
    if(rect.IsIntersect(tileRect))
    {
       ClipNode(*it, rect);
       ++it;
    }
    else
    {
      RemoveTile(*it);
      ClipNode(*it, rect);
      it = node->m_children.erase(it);
    }
  }
}

void TileTree::RemoveTile(TNodePtr const & node)
{
  if (m_removeTile != nullptr && !node->m_isRemoved)
    m_removeTile(node->m_tileKey, node->m_tileStatus);

  node->m_isRemoved = true;
  node->m_tileStatus = TileStatus::Unknown;
}

bool TileTree::ProcessNode(TNodePtr const & node, TileKey const & tileKey,
                           dp::GLState const & state, dp::MasterPointer<dp::RenderBucket> & bucket)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if (tileKey == (*it)->m_tileKey)
    {
      if ((*it)->m_tileStatus == TileStatus::Unknown)
        return false;

      DeleteTilesBelow(*it);

      if (node->m_tileStatus == TileStatus::Rendered)
      {
        (*it)->m_tileStatus = TileStatus::Deferred;
        if (m_deferRenderGroup != nullptr)
          m_deferRenderGroup((*it)->m_tileKey, state, bucket);
        (*it)->m_isRemoved = false;
      }
      else
      {
        (*it)->m_tileStatus = TileStatus::Rendered;
        if (m_addRenderGroup != nullptr)
          m_addRenderGroup((*it)->m_tileKey, state, bucket);
        (*it)->m_isRemoved = false;
      }

      DeleteTilesAbove(node);
      return true;
    }
    else if (IsTileBelow((*it)->m_tileKey, tileKey))
      return ProcessNode(*it, tileKey, state, bucket);
  }
  return false;
}

bool TileTree::FinishNode(TNodePtr const & node, TileKey const & tileKey)
{
  bool changed = false;
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if ((*it)->m_tileKey == tileKey && (*it)->m_tileStatus == TileStatus::Requested)
    {
      // here a tile has finished, but we hadn't got any data from BR. It means that
      // this tile is empty, so we remove all his descendants and him
      (*it)->m_tileStatus = TileStatus::Unknown;
      DeleteTilesBelow(*it);
      changed = true;
    }

    changed |= FinishNode(*it, tileKey);
  }
  return changed;
}

void TileTree::DeleteTilesBelow(TNodePtr const & node)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    RemoveTile(*it);
    DeleteTilesBelow(*it);
  }
  node->m_children.clear();
}

void TileTree::DeleteTilesAbove(TNodePtr const & node)
{
  if (node->m_tileStatus == TileStatus::Requested || node->m_children.empty())
    return;

  // check if all child tiles are ready
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    if ((*it)->m_tileStatus == TileStatus::Requested)
      return;

  // add deferred tiles
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if ((*it)->m_tileStatus == TileStatus::Deferred)
    {
      if (m_addDeferredTile != nullptr)
        m_addDeferredTile((*it)->m_tileKey, (*it)->m_tileStatus);

      (*it)->m_tileStatus = TileStatus::Rendered;
      (*it)->m_isRemoved = false;
    }
  }

  // remove current tile
  if (node != m_root)
    RemoveTile(node);
}

void TileTree::SimplifyTree()
{
  ClearEmptyLevels(m_root);
  ClearObsoleteTiles(m_root);
}

void TileTree::ClearEmptyLevels(TNodePtr const & node)
{
  if (HaveChildrenSameStatus(node, TileStatus::Unknown))
  {
    // all grandchildren have the same zoom level?
    if (!HaveGrandchildrenSameZoomLevel(node))
      return;

    // move grandchildren to grandfather
    list<TNodePtr> newChildren;
    for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    {
      RemoveTile(*it);
      newChildren.splice(newChildren.begin(), (*it)->m_children);
    }
    node->m_children.swap(newChildren);
  }

  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    ClearEmptyLevels(*it);
}

bool TileTree::ClearObsoleteTiles(TNodePtr const & node)
{
  bool canClear = true;
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    canClear &= ClearObsoleteTiles(*it);

  if (canClear)
  {
    for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
      RemoveTile(*it);

    node->m_children.clear();
  }

  return canClear && node->m_tileStatus == TileStatus::Unknown;
}

bool TileTree::HaveChildrenSameStatus(TNodePtr const & node, TileStatus tileStatus) const
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    if ((*it)->m_tileStatus != tileStatus)
      return false;

  return true;
}

bool TileTree::HaveGrandchildrenSameZoomLevel(TNodePtr const & node) const
{
  if (node->m_children.empty())
    return true;

  int zoomLevel = -1;
  for (auto childIt = node->m_children.begin(); childIt != node->m_children.end(); ++childIt)
    if (!(*childIt)->m_children.empty())
      zoomLevel = (*childIt)->m_children.front()->m_tileKey.m_zoomLevel;

  // have got grandchildren?
  if (zoomLevel == -1)
    return true;

  for (auto childIt = node->m_children.begin(); childIt != node->m_children.end(); ++childIt)
    for (auto grandchildIt = (*childIt)->m_children.begin(); grandchildIt != (*childIt)->m_children.end(); ++grandchildIt)
      if (zoomLevel != (*grandchildIt)->m_tileKey.m_zoomLevel)
        return false;

  return true;
}

void TileTree::CheckDeferredTiles(TNodePtr const & node)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    DeleteTilesAbove(*it);
    CheckDeferredTiles(*it);
  }
}

void DebugPrintNode(TileTree::TNodePtr const & node, ostringstream & out, string const & offset)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    out << offset << "{ " << DebugPrint((*it)->m_tileKey) << ", "
        << DebugPrint((*it)->m_tileStatus) << ((*it)->m_isRemoved ? ", removed" : "") << "}\n";
    DebugPrintNode(*it, out, offset + "  ");
  }
}

string DebugPrint(TileTree const & tileTree)
{
  ostringstream out;
  out << "\n{\n";
  DebugPrintNode(tileTree.m_root, out, "  ");
  out << "}\n";
  return out.str();
}

} // namespace df
