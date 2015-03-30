#pragma once

#include "tile_utils.hpp"

#include "../geometry/rect2d.hpp"

#include "../std/function.hpp"
#include "../std/list.hpp"
#include "../std/sstream.hpp"
#include "../std/unique_ptr.hpp"

namespace df
{

class TileTree
{
public:
  TileTree();
  ~TileTree();

  using TTileHandler = function<void(TileKey const &, TileStatus)>;

  void BeginRequesting(int const zoomLevel);
  void RequestTile(TileKey const & tileKey);
  void EndRequesting();

  bool ProcessTile(TileKey const & tileKey, TTileHandler const & addTile,
                   TTileHandler const & removeTile, TTileHandler const & deferTile);
  void ClipByRect(m2::RectD const & rect, TTileHandler const & addTile,
                  TTileHandler const & removeTile);
  void Clear();

  bool ContainsTileKey(TileKey const & tileKey);
  void GetRequestedTiles(TTilesCollection & tiles);

private:
  struct Node;
  using TNodePtr = unique_ptr<Node>;

  void InsertToNode(TNodePtr const & node, TileKey const & tileKey);
  void AbortRequestedTiles(TNodePtr const & node, int const zoomLevel);

  void FindRequestedTiles(TNodePtr const & node, TTilesCollection & tiles);
  bool ContainsTileKey(TNodePtr const & node, TileKey const & tileKey);

  void ClipNode(TNodePtr const & node, m2::RectD const & rect,
                TTileHandler const & removeTile);

  void RemoveTile(TNodePtr const & node, TTileHandler const & removeTile);

  bool ProcessNode(TNodePtr const & node, TileKey const & tileKey,
                   TTileHandler const & addTile, TTileHandler const & removeTile,
                   TTileHandler const & deferTile);

  void DeleteTilesBelow(TNodePtr const & node, TTileHandler const & removeTile);
  void DeleteTilesAbove(TNodePtr const & node, TTileHandler const & addTile,
                        TTileHandler const & removeTile);

  void SimplifyTree(TTileHandler const & removeTile);
  void SimplifyTree(TNodePtr const & node, TTileHandler const & removeTile);
  bool ClearObsoleteTiles(TNodePtr const & node, TTileHandler const & removeTile);
  void CheckDeferredTiles(TNodePtr const & node, TTileHandler const & addTile,
                          TTileHandler const & removeTile);

  bool HaveChildrenSameStatus(TNodePtr const & node, TileStatus tileStatus);

private:
  struct Node
  {
    TileKey m_tileKey;
    list<TNodePtr> m_children;
    TileStatus m_tileStatus = TileStatus::Unknown;
    bool m_isRemoved = false;

    Node() = default;
    Node(TileKey const & key, TileStatus status)
      : m_tileKey(key), m_tileStatus(status)
    {}
  };

  TNodePtr m_root;

private:
  friend void DebugPrintNode(TileTree::TNodePtr const & node, ostringstream & out, string const & offset);
  friend string DebugPrint(TileTree const & tileTree);
};

} // namespace df
