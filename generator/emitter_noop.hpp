#pragma once

#include "emitter_interface.hpp"

#include <string>
#include <vector>

class FeatureBuilder1;
class FeatureParams;

namespace generator
{
class EmitterNoop : public EmitterInterface
{
public:
  // EmitterInterface overrides:
  void Process(FeatureBuilder1 &) override  {}
  bool Finish() override { return true; }
  void GetNames(std::vector<std::string> &) const override {}
};
}  // namespace generator
