#include "generator/emitter_simple.hpp"

#include "generator/feature_builder.hpp"

#include "base/macros.hpp"

namespace generator
{
EmitterSimple::EmitterSimple(feature::GenerateInfo const & info) :
  m_regionGenerator(std::make_unique<SimpleGenerator>(info)) {}

void EmitterSimple::GetNames(std::vector<std::string> & names) const
{
  names = m_regionGenerator->Parent().Names();
}

void EmitterSimple::Process(FeatureBuilder1 & fb)
{
  (*m_regionGenerator)(fb);
}

void EmitterPreserialize::Process(FeatureBuilder1 & fb)
{
  UNUSED_VALUE(fb.PreSerialize());
  EmitterSimple::Process(fb);
}
}  // namespace generator
