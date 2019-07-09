#pragma once

#include "generator/regions/node.hpp"

namespace generator
{
namespace regions
{
class AdminSuburbsMarker
{
public:
  void MarkSuburbs(Node::Ptr & tree);

private:
  void MarkLocality(Node::Ptr & tree);
  void MarkSuburbsInLocality(Node::Ptr & tree);
  void MarkUnderLocalityAsSublocalities(Node::Ptr & tree);
};
}  // namespace regions
}  // namespace generator
