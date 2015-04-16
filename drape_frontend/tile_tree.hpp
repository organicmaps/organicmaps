#pragma once

#include "tile_utils.hpp"
#include "render_group.hpp"

#include "drape/glstate.hpp"
#include "drape/pointers.hpp"
#include "geometry/rect2d.hpp"

#include "std/function.hpp"
#include "std/list.hpp"
#include "std/sstream.hpp"
#include "std/unique_ptr.hpp"

namespace df
{

/// This class implements K-d tree of visible tiles.
class TileTree
{
public:
  TileTree();
  ~TileTree();

  using TTileHandler = function<void(TileKey const &)>;
  using TRenderGroupHandler = function<void(TileKey const &, dp::GLState const &,
                                            drape_ptr<dp::RenderBucket> &&)>;

  /// This method sets following handlers:
  /// addRenderGroup is called when render group can be created by a tile and rendered at once,
  /// deferRenderGroup is called when render group can be created by a tile but has to be deferred,
  /// activateTile is called when previously deferred tile can be rendered,
  /// removeTile is called when a tile must be removed from rendering.
  void SetHandlers(TRenderGroupHandler const & addRenderGroup,
                   TRenderGroupHandler const & deferRenderGroup,
                   TTileHandler const & activateTile,
                   TTileHandler const & removeTile);

  /// This method must be called before requesting bunch of tiles.
  void BeginRequesting(int const zoomLevel, m2::RectD const & clipRect);
  /// This method requests a new tile.
  void RequestTile(TileKey const & tileKey);
  /// This method must be called after requesting bunch of tiles.
  void EndRequesting();

  /// This method processes received from BR (backend renderer) tile.
  void ProcessTile(TileKey const & tileKey, int const zoomLevel,
                   dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket);
  /// This method processes a message about finishing reading tiles on BR.
  void FinishTiles(TTilesCollection const & tiles, int const zoomLevel);

  /// It returns actual tile collection to send to BR.
  void GetTilesCollection(TTilesCollection & tiles, int const zoomLevel) const;

private:
  struct Node;
  using TNodePtr = unique_ptr<Node>;

  void ResetHandlers();

  void InsertToNode(TNodePtr const & node, TileKey const & tileKey);
  void InsertToCurrentNode(TNodePtr const & node, TileKey const & tileKey);
  void InsertToNodeAbove(TNodePtr const & node, TileKey const & tileKey);
  void InsertToNodeBelow(TNodePtr const & node, TileKey const & tileKey, int const childrenZoomLevel);
  void AbortTiles(TNodePtr const & node, int const zoomLevel);

  void ClipByRect(m2::RectD const & rect);

  void FillTilesCollection(TNodePtr const & node, TTilesCollection & tiles, int const zoomLevel) const;

  void ClipNode(TNodePtr const & node, m2::RectD const & rect);
  void CheckDeferredTiles(TNodePtr const & node);

  void RemoveTile(TNodePtr const & node);

  bool ProcessNode(TNodePtr const & node, TileKey const & tileKey, int const zoomLevel,
                   dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket);
  bool FinishNode(TNodePtr const & node, TileKey const & tileKey, int const zoomLevel);

  void DeleteTilesBelow(TNodePtr const & node);
  void TryDeleteTileAbove(TNodePtr const & node);

  void SimplifyTree();
  void ClearEmptyLevels(TNodePtr const & node);
  bool ClearObsoleteTiles(TNodePtr const & node);

  bool HaveChildrenSameStatus(TNodePtr const & node, TileStatus tileStatus) const;
  bool HaveGrandchildrenSameZoomLevel(TNodePtr const & node) const;

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
  TRenderGroupHandler m_addRenderGroupHandler;
  TRenderGroupHandler m_deferRenderGroupHandler;
  TTileHandler m_activateTileHandler;
  TTileHandler m_removeTileHandler;

  friend void DebugPrintNode(TileTree::TNodePtr const & node, ostringstream & out, string const & offset);
  friend string DebugPrint(TileTree const & tileTree);
};

} // namespace df
