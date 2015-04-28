#include "drape_frontend/tile_tree_builder.hpp"

#include "std/algorithm.hpp"

namespace df
{

TileTreeBuilderNode::TileTreeBuilderNode()
  : m_tileStatus(TileStatus::Unknown)
  , m_isRemoved(false)
  , m_prev(nullptr)
{
}

TileTreeBuilderNode::TileTreeBuilderNode(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved)
  : m_tileKey(tileKey)
  , m_tileStatus(tileStatus)
  , m_isRemoved(isRemoved)
  , m_prev(nullptr)
{
}

TileTreeBuilderNode::TileTreeBuilderNode(TileTreeBuilderNode & node)
  : m_next(move(node.m_next))
  , m_firstChild(move(node.m_firstChild))
  , m_tileKey(node.m_tileKey)
  , m_tileStatus(node.m_tileStatus)
  , m_isRemoved(node.m_isRemoved)
  , m_prev(node.m_prev)
{
}

TileTreeBuilderNode & TileTreeBuilderNode::Node(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved)
{
  m_next.reset(new TileTreeBuilderNode(tileKey, tileStatus, isRemoved));
  m_next->m_prev = this;
  return *m_next.get();
}

TileTreeBuilderNode & TileTreeBuilderNode::Children(TileTreeBuilderNode & node)
{
  m_firstChild.reset(new TileTreeBuilderNode(node));
  return *this;
}

TileTreeBuilderNode Node(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved)
{
  TileTreeBuilderNode node(tileKey, tileStatus, isRemoved);
  return node;
}

unique_ptr<TileTree> TileTreeBuilder::Build(TileTreeBuilderNode const & root)
{
  unique_ptr<TileTree> tree = make_unique<TileTree>();
  InsertIntoNode(tree->m_root, root);
  return tree;
}

void TileTreeBuilder::InsertIntoNode(TileTree::TNodePtr & node, TileTreeBuilderNode const & builderNode)
{
  node->m_children.push_back(unique_ptr<TileTree::Node>(new TileTree::Node(builderNode.m_tileKey, builderNode.m_tileStatus, builderNode.m_isRemoved)));
  if (builderNode.m_firstChild != nullptr)
    InsertIntoNode(node->m_children.back(), *builderNode.m_firstChild.get());

  TileTreeBuilderNode * n = builderNode.m_prev;
  while (n != nullptr)
  {
    node->m_children.push_back(unique_ptr<TileTree::Node>(new TileTree::Node(n->m_tileKey, n->m_tileStatus, n->m_isRemoved)));
    if (n->m_firstChild != nullptr)
      InsertIntoNode(node->m_children.back(), *n->m_firstChild.get());
    n = n->m_prev;
  }
}

bool TileTreeComparer::IsEqual(unique_ptr<TileTree> const & tree1, unique_ptr<TileTree> const & tree2)
{
  return CompareSubtree(tree1->m_root, tree2->m_root);
}

bool TileTreeComparer::CompareSubtree(TileTree::TNodePtr const & node1, TileTree::TNodePtr const & node2)
{
  if (!CompareNodes(node1, node2))
    return false;

  for (TileTree::TNodePtr const & n1 : node1->m_children)
  {
    auto found = find_if(node2->m_children.begin(), node2->m_children.end(), [this, &n1](TileTree::TNodePtr const & n2)
    {
      return CompareNodes(n1, n2);
    });

    if (found != node2->m_children.end())
    {
      if (!CompareSubtree(n1, *found))
        return false;
    }
    else
    {
      return false;
    }
  }

  return true;
}

bool TileTreeComparer::CompareNodes(TileTree::TNodePtr const & node1, TileTree::TNodePtr const & node2)
{
  if (!(node1->m_tileKey == node2->m_tileKey) || node1->m_tileStatus != node2->m_tileStatus ||
      node1->m_isRemoved != node2->m_isRemoved || node1->m_children.size() != node2->m_children.size())
    return false;

  return true;
}

} //namespace df
