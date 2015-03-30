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
}

void TileTree::Clear()
{
  m_root.reset(new Node());
}

void TileTree::BeginRequesting(int const zoomLevel)
{
  AbortRequestedTiles(m_root, zoomLevel);
}

void TileTree::RequestTile(TileKey const & tileKey)
{
  InsertToNode(m_root, tileKey);
}

void TileTree::EndRequesting()
{
  SimplifyTree(nullptr);
}

void TileTree::GetRequestedTiles(TTilesCollection & tiles)
{
  FindRequestedTiles(m_root, tiles);
}

bool TileTree::ContainsTileKey(TileKey const & tileKey)
{
  return ContainsTileKey(m_root, tileKey);
}

void TileTree::ClipByRect(m2::RectD const & rect, TTileHandler const & addTile,
                          TTileHandler const & removeTile)
{
  ClipNode(m_root, rect, removeTile);
  CheckDeferredTiles(m_root, addTile, removeTile);
  SimplifyTree(removeTile);
}

bool TileTree::ProcessTile(TileKey const & tileKey, TTileHandler const & addTile,
                           TTileHandler const & removeTile, TTileHandler const & deferTile)
{
  bool const result = ProcessNode(m_root, tileKey, addTile, removeTile, deferTile);
  if (result)
      SimplifyTree(removeTile);

  return result;
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

void TileTree::AbortRequestedTiles(TNodePtr const & node, int const zoomLevel)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if ((*it)->m_tileStatus == TileStatus::Requested && (*it)->m_tileKey.m_zoomLevel != zoomLevel)
      (*it)->m_tileStatus = TileStatus::Unknown;

    AbortRequestedTiles(*it, zoomLevel);
  }
}

void TileTree::FindRequestedTiles(TNodePtr const & node, TTilesCollection & tiles)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if ((*it)->m_tileStatus == TileStatus::Requested)
      tiles.insert((*it)->m_tileKey);

    FindRequestedTiles(*it, tiles);
  }
}

bool TileTree::ContainsTileKey(TNodePtr const & node, TileKey const & tileKey)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if (tileKey == (*it)->m_tileKey)
      return (*it)->m_tileStatus != TileStatus::Unknown;
    else if (IsTileBelow((*it)->m_tileKey, tileKey))
      return ContainsTileKey(*it, tileKey);
  }
  return false;
}

void TileTree::ClipNode(TNodePtr const & node, m2::RectD const & rect, TTileHandler const & removeTile)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end();)
  {
    m2::RectD const tileRect = (*it)->m_tileKey.GetGlobalRect();
    if(rect.IsIntersect(tileRect))
    {
       ClipNode(*it, rect, removeTile);
       ++it;
    }
    else
    {
      RemoveTile(*it, removeTile);
      ClipNode(*it, rect, removeTile);
      it = node->m_children.erase(it);
    }
  }
}

void TileTree::RemoveTile(TNodePtr const & node, TTileHandler const & removeTile)
{
  if (removeTile != nullptr && !node->m_isRemoved)
    removeTile(node->m_tileKey, node->m_tileStatus);

  node->m_isRemoved = true;
  node->m_tileStatus = TileStatus::Unknown;
}

bool TileTree::ProcessNode(TNodePtr const & node, TileKey const & tileKey,
                           TTileHandler const & addTile, TTileHandler const & removeTile,
                           TTileHandler const & deferTile)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    if (tileKey == (*it)->m_tileKey)
    {
      if ((*it)->m_tileStatus == TileStatus::Unknown)
        return false;

      DeleteTilesBelow(*it, removeTile);

      if (node->m_tileStatus == TileStatus::Rendered)
      {
        (*it)->m_tileStatus = TileStatus::Deferred;
        if (deferTile != nullptr)
          deferTile((*it)->m_tileKey, (*it)->m_tileStatus);
        (*it)->m_isRemoved = false;
      }
      else
      {
        (*it)->m_tileStatus = TileStatus::Rendered;
        if (addTile != nullptr)
          addTile((*it)->m_tileKey, (*it)->m_tileStatus);
        (*it)->m_isRemoved = false;
      }

      DeleteTilesAbove(node, addTile, removeTile);
      return true;
    }
    else if (IsTileBelow((*it)->m_tileKey, tileKey))
      return ProcessNode(*it, tileKey, addTile, removeTile, deferTile);
  }
  return false;
}

void TileTree::DeleteTilesBelow(TNodePtr const & node, TTileHandler const & removeTile)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    RemoveTile(*it, removeTile);
    DeleteTilesBelow(*it, removeTile);
  }
  node->m_children.clear();
}

void TileTree::DeleteTilesAbove(TNodePtr const & node, TTileHandler const & addTile, TTileHandler const & removeTile)
{
  if (node->m_tileStatus == TileStatus::Requested)
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
      if (addTile != nullptr)
        addTile((*it)->m_tileKey, (*it)->m_tileStatus);

      (*it)->m_tileStatus = TileStatus::Rendered;
      (*it)->m_isRemoved = false;
    }
  }

  // remove current tile
  if (node != m_root)
    RemoveTile(node, removeTile);
}

void TileTree::SimplifyTree(TTileHandler const & removeTile)
{
  SimplifyTree(m_root, removeTile);
  ClearObsoleteTiles(m_root, removeTile);
}

void TileTree::SimplifyTree(TNodePtr const & node, TTileHandler const & removeTile)
{
  if (HaveChildrenSameStatus(node, TileStatus::Unknown))
  {
    // all children of children are rendered
    for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
      if (!HaveChildrenSameStatus(*it, TileStatus::Rendered))
        return;

    // move children to grandfather
    list<TNodePtr> newChildren;
    for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    {
      RemoveTile(*it, removeTile);
      newChildren.splice(newChildren.begin(), (*it)->m_children);
    }

    node->m_children.swap(newChildren);
  }

  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    SimplifyTree(*it, removeTile);
}

bool TileTree::ClearObsoleteTiles(TNodePtr const & node, TTileHandler const & removeTile)
{
  bool canClear = true;
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    canClear &= ClearObsoleteTiles(*it, removeTile);

  if (canClear)
  {
    for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
      RemoveTile(*it, removeTile);

    node->m_children.clear();
  }

  return canClear && node->m_tileStatus == TileStatus::Unknown;
}

bool TileTree::HaveChildrenSameStatus(TNodePtr const & node, TileStatus tileStatus)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
    if ((*it)->m_tileStatus != tileStatus)
      return false;

  return true;
}

void TileTree::CheckDeferredTiles(TNodePtr const & node, TTileHandler const & addTile, TTileHandler const & removeTile)
{
  for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it)
  {
    DeleteTilesAbove(node, addTile, removeTile);
    CheckDeferredTiles(*it, addTile, removeTile);
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
