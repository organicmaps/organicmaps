#pragma once

namespace feature
{
class FeatureBuilder;
}  // namespace feature

class FeatureEmitterIFace
{
public:
  virtual ~FeatureEmitterIFace() {}
  virtual void operator() (feature::FeatureBuilder const &) = 0;
};
