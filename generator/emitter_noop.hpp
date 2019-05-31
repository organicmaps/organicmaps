#pragma once

#include "emitter_interface.hpp"

#include <string>
#include <vector>

class FeatureParams;

namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
class EmitterNoop : public EmitterInterface
{
public:
  // EmitterInterface overrides:
  void Process(feature::FeatureBuilder &) override  {}
  bool Finish() override { return true; }
  void GetNames(std::vector<std::string> &) const override {}
};
}  // namespace generator
