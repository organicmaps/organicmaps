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

  void BeginRequesting(int const zoomLevel, TTileHandler const & removeTile);
  void RequestTile(TileKey const & tileKey);
  void EndRequesting(TTileHandler const & removeTile);

  bool ProcessTile(TileKey const & tileKey, TTileHandler const & addTile,
                   TTileHandler const & removeTile, TTileHandler const & deferTile);

  void FinishTile(TileKey const & tileKey, TTileHandler const & addDeferredTile,
                  TTileHandler const & removeTile);

  void ClipByRect(m2::RectD const & rect, TTileHandler const & addDeferredTile,
                  TTileHandler const & removeTile);
  void Clear();

  void GetTilesCollection(TTilesCollection & tiles, int const zoomLevel);

private:
  struct Node;
  using TNodePtr = unique_ptr<Node>;

  void InsertToNode(TNodePtr const & node, TileKey const & tileKey);
  void AbortTiles(TNodePtr const & node, int const zoomLevel, TTileHandler const & removeTile);

  void FillTilesCollection(TNodePtr const & node, TTilesCollection & tiles, int const zoomLevel);

  void ClipNode(TNodePtr const & node, m2::RectD const & rect,
                TTileHandler const & removeTile);
  void CheckDeferredTiles(TNodePtr const & node, TTileHandler const & addTile,
                          TTileHandler const & removeTile);

  void RemoveTile(TNodePtr const & node, TTileHandler const & removeTile);

  bool ProcessNode(TNodePtr const & node, TileKey const & tileKey,
                   TTileHandler const & addTile, TTileHandler const & removeTile,
                   TTileHandler const & deferTile);
  bool FinishNode(TNodePtr const & node, TileKey const & tileKey);

  void DeleteTilesBelow(TNodePtr const & node, TTileHandler const & removeTile);
  void DeleteTilesAbove(TNodePtr const & node, TTileHandler const & addTile,
                        TTileHandler const & removeTile);

  void SimplifyTree(TTileHandler const & removeTile);
  void ClearEmptyLevels(TNodePtr const & node, TTileHandler const & removeTile);
  bool ClearObsoleteTiles(TNodePtr const & node, TTileHandler const & removeTile);

  bool HaveChildrenSameStatus(TNodePtr const & node, TileStatus tileStatus) const;
  bool HaveGrandchildrenSameZoomLevel(TNodePtr const & node) const;

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
