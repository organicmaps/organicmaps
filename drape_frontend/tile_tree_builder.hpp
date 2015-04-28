#pragma once

#include "drape_frontend/tile_tree.hpp"

#include "std/list.hpp"
#include "std/unique_ptr.hpp"

namespace df
{

// These classes can be used only for tests!

class TileTreeBuilderNode
{
  friend class TileTreeBuilder;
public:
  TileTreeBuilderNode & Node(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved = false);
  TileTreeBuilderNode & Children(TileTreeBuilderNode & node);

private:
  TileTreeBuilderNode();
  TileTreeBuilderNode(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved);
  TileTreeBuilderNode(TileTreeBuilderNode & node);

  TileTreeBuilderNode * m_prev;
  unique_ptr<TileTreeBuilderNode> m_next;
  unique_ptr<TileTreeBuilderNode> m_firstChild;

  TileKey m_tileKey;
  TileStatus m_tileStatus;
  bool m_isRemoved;

  friend TileTreeBuilderNode Node(TileKey const &, TileStatus, bool);
};

TileTreeBuilderNode Node(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved = false);

class TileTreeBuilder
{
public:
  unique_ptr<TileTree> Build(TileTreeBuilderNode const & root);

private:
  void InsertIntoNode(TileTree::TNodePtr & node, TileTreeBuilderNode const & builderNode);
};

class TileTreeComparer
{
public:
  bool IsEqual(unique_ptr<TileTree> const & tree1, unique_ptr<TileTree> const & tree2);

private:
  bool CompareSubtree(TileTree::TNodePtr const & node1, TileTree::TNodePtr const & node2);
  bool CompareNodes(TileTree::TNodePtr const & node1, TileTree::TNodePtr const & node2);
};

} // namespace df
