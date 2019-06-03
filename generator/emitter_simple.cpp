#include "generator/emitter_simple.hpp"

#include "generator/feature_builder.hpp"

#include "base/macros.hpp"

namespace generator
{
EmitterSimple::EmitterSimple(feature::GenerateInfo const & info) :
  m_regionGenerator(std::make_unique<SimpleGenerator>(info)) {}

void EmitterSimple::GetNames(std::vector<std::string> & names) const
{
  names = m_regionGenerator->Parent().GetNames();
}

void EmitterSimple::Process(FeatureBuilder1 & fb)
{
  auto & polygonizer = m_regionGenerator->Parent();
  // Emit each feature independently: clear current country names (see Polygonizer::GetCurrentNames()).
  polygonizer.Start();
  (*m_regionGenerator)(fb);
  polygonizer.Finish();
}

void EmitterPreserialize::Process(FeatureBuilder1 & fb)
{
  UNUSED_VALUE(fb.PreSerialize());
  EmitterSimple::Process(fb);
}
}  // namespace generator
