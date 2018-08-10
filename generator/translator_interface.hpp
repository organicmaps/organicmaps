#pragma once

struct OsmElement;

namespace generator
{
// Osm to feature translator interface.
class TranslatorInterface
{
public:
  virtual ~TranslatorInterface() {}

  virtual void EmitElement(OsmElement * p) = 0;
};
}  // namespace generator
