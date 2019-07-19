#include "generator/regions/node.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <numeric>

namespace generator
{
namespace regions
{
size_t TreeSize(Node::Ptr const & node)
{
  if (node == nullptr)
    return 0;

  size_t size = 1;
  for (auto const & n : node->GetChildren())
    size += TreeSize(n);

  return size;
}

size_t MaxDepth(Node::Ptr const & node)
{
  if (node == nullptr)
    return 0;

  size_t depth = 1;
  for (auto const & n : node->GetChildren())
    depth = std::max(MaxDepth(n), depth);

  return depth;
}

NodePath MakeLevelPath(Node::Ptr const & node)
{
  CHECK(node->GetData().GetLevel() != PlaceLevel::Unknown, ());

  std::array<bool, static_cast<std::size_t>(PlaceLevel::Count)> skipLevels{};
  NodePath path{node};
  for (auto p = node->GetParent(); p; p = p->GetParent())
  {
    auto const level = p->GetData().GetLevel();
    if (PlaceLevel::Unknown == level)
      continue;

    auto levelIndex = static_cast<std::size_t>(level);
    if (skipLevels.at(levelIndex))
      continue;

    skipLevels[levelIndex] = true;
    if (PlaceLevel::Locality == level)
    {
      // To ignore covered locality.
      skipLevels[static_cast<std::size_t>(PlaceLevel::Suburb)] = true;
      skipLevels[static_cast<std::size_t>(PlaceLevel::Sublocality)] = true;
    }

    path.push_back(p);
  }
  std::reverse(path.begin(), path.end());

  return path;
}

void PrintTree(Node::Ptr const & node, std::ostream & stream = std::cout, std::string prefix = "",
               bool isTail = true)
{
  auto const & children = node->GetChildren();
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
  auto const center = MercatorBounds::ToLatLon({point.get<0>(), point.get<1>()});
  auto const label = GetLabel(d.GetLevel());
  stream << d.GetName() << "<"
         << d.GetTranslatedOrTransliteratedName(StringUtf8Multilang::GetLangIndex("en")) << "> ("
         << DebugPrint(d.GetId()) << ";" << (label ? label : "-") << ";"
         << static_cast<size_t>(d.GetRank()) << ";[" << std::fixed << std::setprecision(7)
         << center.m_lat << "," << center.m_lon << "])" << std::endl;
  for (size_t i = 0, size = children.size(); i < size; ++i)
    PrintTree(children[i], stream, prefix, i == size - 1);
}

void DebugPrintTree(Node::Ptr const & tree, std::ostream & stream)
{
  stream << "ROOT NAME: " << tree->GetData().GetName() << std::endl;
  stream << "MAX DEPTH: " << MaxDepth(tree) << std::endl;
  stream << "TREE SIZE: " << TreeSize(tree) << std::endl;
  PrintTree(tree, stream);
  stream << std::endl;
}
}  // namespace regions
}  // namespace generator
