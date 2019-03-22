#pragma once

#include <string>
#include <vector>

struct OsmElement;

namespace generator
{
// Implementing this interface allows an object to create intermediate data from OsmElement.
class TranslatorInterface
{
public:
  virtual ~TranslatorInterface() = default;

  virtual void Preprocess(OsmElement & element) {}
  virtual void Emit(OsmElement & element) = 0;
  virtual bool Finish() = 0;
  virtual void GetNames(std::vector<std::string> & names) const = 0;
};
}  // namespace generator
