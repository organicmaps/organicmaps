#pragma once

#include <string>
#include <vector>

class FeatureBuilder1;
class FeatureParams;

namespace generator
{
// Emitter is used in OsmElement to FeatureBuilder translation process.
class EmitterInterface
{
public:
  virtual ~EmitterInterface() = default;

  /// This method is used by OsmTranslator to pass |fb| to Emitter for further processing.
  virtual void operator()(FeatureBuilder1 & fb) = 0;
  virtual void EmitCityBoundary(FeatureBuilder1 const & fb, FeatureParams const & params) {}
  /// Finish is used in GenerateFeatureImpl to make whatever work is needed after
  /// all OsmElements are processed.
  virtual bool Finish() = 0;
  /// Sets buckets (mwm names).
  // TODO(syershov): Make this topic clear.
  virtual void GetNames(std::vector<std::string> & names) const = 0;
};
}  // namespace generator
