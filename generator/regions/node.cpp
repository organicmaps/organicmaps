#include "generator/regions/node.hpp"

#include <algorithm>
#include <numeric>

namespace generator
{
namespace regions
{
namespace
{
using MergeFunc = std::function<Node::Ptr(Node::Ptr, Node::Ptr)>;

bool LessNodePtrById(Node::Ptr l, Node::Ptr r)
{
  auto const & lRegion = l->GetData();
  auto const & rRegion = r->GetData();
  return lRegion.GetId() < rRegion.GetId();
}

Node::PtrList MergeChildren(Node::PtrList const & l, Node::PtrList const & r, Node::Ptr newParent)
{
  Node::PtrList result(l);
  std::copy(std::begin(r), std::end(r), std::back_inserter(result));
  for (auto & p : result)
    p->SetParent(newParent);

  std::sort(std::begin(result), std::end(result), LessNodePtrById);
  return result;
}

Node::PtrList NormalizeChildren(Node::PtrList const & children, MergeFunc mergeTree)
{
  Node::PtrList uniqueChildren;
  auto const pred = [](Node::Ptr l, Node::Ptr r)
  {
    auto const & lRegion = l->GetData();
    auto const & rRegion = r->GetData();
    return lRegion.GetId() == rRegion.GetId();
  };
  std::unique_copy(std::begin(children), std::end(children),
                   std::back_inserter(uniqueChildren), pred);
  Node::PtrList result;
  for (auto const & ch : uniqueChildren)
  {
    auto const bounds = std::equal_range(std::begin(children), std::end(children), ch,
                                         LessNodePtrById);
    auto merged = std::accumulate(bounds.first, bounds.second, Node::Ptr(), mergeTree);
    result.emplace_back(std::move(merged));
  }

  return result;
}

Node::Ptr MergeHelper(Node::Ptr l, Node::Ptr r, MergeFunc mergeTree)
{
  auto const & lChildren = l->GetChildren();
  auto const & rChildren = r->GetChildren();
  auto const children = MergeChildren(lChildren, rChildren, l);
  if (children.empty())
    return l;

  auto resultChildren = NormalizeChildren(children, mergeTree);
  l->SetChildren(std::move(resultChildren));
  r->RemoveChildren();
  return l;
}
}  // nmespace

size_t TreeSize(Node::Ptr node)
{
  if (node == nullptr)
    return 0;

  size_t size = 1;
  for (auto const & n : node->GetChildren())
    size += TreeSize(n);

  return size;
}

size_t MaxDepth(Node::Ptr node)
{
  if (node == nullptr)
    return 0;

  size_t depth = 1;
  for (auto const & n : node->GetChildren())
    depth = std::max(MaxDepth(n), depth);

  return depth;
}

void PrintTree(Node::Ptr node, std::ostream & stream = std::cout, std::string prefix = "",
               bool isTail = true)
{
  auto const & childern = node->GetChildren();
  stream << prefix;
  if (isTail)
  {
    stream << "└───";
    prefix += "    ";
  }
  else
  {
    stream << "├───";
    prefix += "│   ";
  }

  auto const & d = node->GetData();
  auto const point = d.GetCenter();
  stream << d.GetName() << "<" << d.GetEnglishOrTransliteratedName() << "> ("
         << d.GetId()
         << ";" << d.GetLabel()
         << ";" << static_cast<size_t>(d.GetRank())
         << ";[" << point.get<0>() << "," << point.get<1>() << "])"
         << std::endl;
  for (size_t i = 0, size = childern.size(); i < size; ++i)
    PrintTree(childern[i], stream, prefix, i == size - 1);
}

void DebugPrintTree(Node::Ptr tree, std::ostream & stream)
{
  stream << "ROOT NAME: " << tree->GetData().GetName() << std::endl;
  stream << "MAX DEPTH: " <<  MaxDepth(tree) << std::endl;
  stream << "TREE SIZE: " <<  TreeSize(tree) << std::endl;
  PrintTree(tree, stream);
  stream << std::endl;
}

Node::Ptr MergeTree(Node::Ptr l, Node::Ptr r)
{
  if (l == nullptr)
    return r;

  if (r == nullptr)
    return l;

  auto const & lRegion = l->GetData();
  auto const & rRegion = r->GetData();
  if (lRegion.GetId() != rRegion.GetId())
    return nullptr;

  if (lRegion.GetArea() > rRegion.GetArea())
    return MergeHelper(l, r, MergeTree);
  else
    return MergeHelper(r, l, MergeTree);
}

void NormalizeTree(Node::Ptr tree)
{
  if (tree == nullptr)
    return;

  auto & children = tree->GetChildren();
  std::sort(std::begin(children), std::end(children), LessNodePtrById);
  auto newChildren = NormalizeChildren(children, MergeTree);
  tree->SetChildren(std::move(newChildren));
  for (auto const & ch : tree->GetChildren())
    NormalizeTree(ch);
}
}  // namespace regions
}  // namespace generator
