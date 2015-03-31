#pragma once

#include "tile_utils.hpp"
#include "render_group.hpp"

#include "../drape/glstate.hpp"
#include "../drape/pointers.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/function.hpp"
#include "../std/list.hpp"
#include "../std/sstream.hpp"
#include "../std/unique_ptr.hpp"

namespace df
{

/// this class implements K-d tree of visible tiles
class TileTree
{
public:
  TileTree();
  ~TileTree();

  using TTileHandler = function<void(TileKey const &, TileStatus)>;
  using TRenderGroupHandler = function<void(TileKey const &, dp::GLState const &,
                                            dp::MasterPointer<dp::RenderBucket> &)>;

  /// addRenderGroup is called when render group can be created by a tile and rendered at once
  /// deferRenderGroup is called when render group can be created by a tile but has to be deferred
  /// addDeferredTile is called when previously deferred tile can be rendered
  /// removeTile is called when a tile must be removed from rendering
  void SetHandlers(TRenderGroupHandler const & addRenderGroup,
                   TRenderGroupHandler const & deferRenderGroup,
                   TTileHandler const & addDeferredTile,
                   TTileHandler const & removeTile);
  void ResetHandlers();

  /// this method must be called before requesting bunch of tiles
  void BeginRequesting(int const zoomLevel);
  /// request a new tile
  void RequestTile(TileKey const & tileKey);
  /// this method must be called after requesting bunch of tiles
  void EndRequesting();

  /// this method processes received from BR tile
  bool ProcessTile(TileKey const & tileKey, int const zoomLevel,
                   dp::GLState const & state, dp::MasterPointer<dp::RenderBucket> & bucket);
  /// this method processes a message about finishing tile on BR
  void FinishTile(TileKey const & tileKey, int const zoomLevel);

  /// this method performs clipping by rectangle
  void ClipByRect(m2::RectD const & rect);

  /// clear all entire structure of tree. DO NOT call removeTile handlers,
  /// so all external storages of tiles must be cleared manually
  void Clear();

  /// it returns actual tile collection to send to BR
  void GetTilesCollection(TTilesCollection & tiles, int const zoomLevel);

private:
  struct Node;
  using TNodePtr = unique_ptr<Node>;

  void InsertToNode(TNodePtr const & node, TileKey const & tileKey);
  void AbortTiles(TNodePtr const & node, int const zoomLevel);

  void FillTilesCollection(TNodePtr const & node, TTilesCollection & tiles, int const zoomLevel);

  void ClipNode(TNodePtr const & node, m2::RectD const & rect);
  void CheckDeferredTiles(TNodePtr const & node);

  void RemoveTile(TNodePtr const & node);

  bool ProcessNode(TNodePtr const & node, TileKey const & tileKey, int const zoomLevel,
                   dp::GLState const & state, dp::MasterPointer<dp::RenderBucket> & bucket);
  bool FinishNode(TNodePtr const & node, TileKey const & tileKey, int const zoomLevel);

  void DeleteTilesBelow(TNodePtr const & node);
  void DeleteTilesAbove(TNodePtr const & node);

  void SimplifyTree();
  void ClearEmptyLevels(TNodePtr const & node);
  bool ClearObsoleteTiles(TNodePtr const & node);

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
  TRenderGroupHandler m_addRenderGroup;
  TRenderGroupHandler m_deferRenderGroup;
  TTileHandler m_addDeferredTile;
  TTileHandler m_removeTile;

private:
  friend void DebugPrintNode(TileTree::TNodePtr const & node, ostringstream & out, string const & offset);
  friend string DebugPrint(TileTree const & tileTree);
};

} // namespace df
