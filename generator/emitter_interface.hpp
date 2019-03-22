#pragma once

#include <string>
#include <vector>

class FeatureBuilder1;
class FeatureParams;

namespace generator
{
// Implementing this interface allows an object to process FeatureBuilder1 objects and broadcast them.
class EmitterInterface
{
public:
  virtual ~EmitterInterface() = default;

  // This method is used by OsmTranslator to pass |fb| to Emitter for further processing.
  virtual void Process(FeatureBuilder1 & fb) = 0;
  // Finish is used in GenerateFeatureImpl to make whatever work is needed after all OsmElements
  // are processed.
  virtual bool Finish() = 0;
  // Sets buckets (mwm names).
  // TODO(syershov): Make this topic clear.
  virtual void GetNames(std::vector<std::string> & names) const = 0;
};
}  // namespace generator
