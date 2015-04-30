#include "drape_frontend/tile_tree_builder.hpp"

#include "std/algorithm.hpp"

namespace df
{

TileTreeBuilderNode::TileTreeBuilderNode()
  : m_prevBrother(nullptr)
  , m_tileStatus(TileStatus::Unknown)
  , m_isRemoved(false)
{
}

TileTreeBuilderNode::TileTreeBuilderNode(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved)
  : m_prevBrother(nullptr)
  , m_tileKey(tileKey)
  , m_tileStatus(tileStatus)
  , m_isRemoved(isRemoved)
{
}

TileTreeBuilderNode::TileTreeBuilderNode(TileTreeBuilderNode & node)
  : m_prevBrother(node.m_prevBrother)
  , m_nextBrother(move(node.m_nextBrother))
  , m_child(move(node.m_child))
  , m_tileKey(node.m_tileKey)
  , m_tileStatus(node.m_tileStatus)
  , m_isRemoved(node.m_isRemoved)
{
}

TileTreeBuilderNode & TileTreeBuilderNode::Node(TileKey const & tileKey, TileStatus tileStatus, bool isRemoved)
{
  m_nextBrother.reset(new TileTreeBuilderNode(tileKey, tileStatus, isRemoved));
  m_nextBrother->m_prevBrother = this;
  return *m_nextBrother.get();
}

TileTreeBuilderNode & TileTreeBuilderNode::Children(TileTreeBuilderNode & node)
{
  m_child.reset(new TileTreeBuilderNode(node));
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
  node->m_children.push_back(CreateNode(& builderNode));
  if (builderNode.m_child != nullptr)
    InsertIntoNode(node->m_children.back(), *builderNode.m_child.get());

  TileTreeBuilderNode * n = builderNode.m_prevBrother;
  while (n != nullptr)
  {
    node->m_children.push_back(CreateNode(n));
    if (n->m_child != nullptr)
      InsertIntoNode(node->m_children.back(), *n->m_child.get());
    n = n->m_prevBrother;
  }
}

unique_ptr<TileTree::Node> TileTreeBuilder::CreateNode(TileTreeBuilderNode const * node)
{
  return make_unique<TileTree::Node>(node->m_tileKey, node->m_tileStatus, node->m_isRemoved);
}

bool TileTreeComparer::IsEqual(unique_ptr<TileTree> const & tree1, unique_ptr<TileTree> const & tree2) const
{
  return CompareSubtree(tree1->m_root, tree2->m_root);
}

bool TileTreeComparer::CompareSubtree(TileTree::TNodePtr const & node1, TileTree::TNodePtr const & node2) const
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

bool TileTreeComparer::CompareNodes(TileTree::TNodePtr const & node1, TileTree::TNodePtr const & node2) const
{
  if (!(node1->m_tileKey == node2->m_tileKey) || node1->m_tileStatus != node2->m_tileStatus ||
      node1->m_isRemoved != node2->m_isRemoved || node1->m_children.size() != node2->m_children.size())
    return false;

  return true;
}

} //namespace df
