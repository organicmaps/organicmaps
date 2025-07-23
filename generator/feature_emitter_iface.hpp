#pragma once

namespace feature
{
class FeatureBuilder;
}  // namespace feature

class FeatureEmitterIFace
{
  // Disable deletion via this interface, because some dtors in derived classes are noexcept(false).

protected:
  ~FeatureEmitterIFace() = default;

public:
  virtual void operator()(feature::FeatureBuilder const &) = 0;
};
