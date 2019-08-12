#pragma once

#include "generator/regions/node.hpp"

namespace generator
{
namespace regions
{
class AdminSuburbsMarker
{
public:
  static constexpr double kLocalityAreaRatioMax = 0.8;

  void MarkSuburbs(Node::Ptr & tree);

private:
  void MarkLocality(Node::Ptr & tree);
  void MarkSuburbsInLocality(Node::Ptr & tree, LevelRegion const & locality);
  void MarkUnderLocalityAsSublocalities(Node::Ptr & tree);
};
}  // namespace regions
}  // namespace generator
