#pragma once

#include <string>
#include <vector>

struct OsmElement;

namespace generator
{
// Osm to feature translator interface.
class TranslatorInterface
{
public:
  virtual ~TranslatorInterface() {}

  virtual void EmitElement(OsmElement * p) = 0;
  virtual bool Finish() = 0;
  virtual void GetNames(std::vector<std::string> & names) const = 0;
};
}  // namespace generator
