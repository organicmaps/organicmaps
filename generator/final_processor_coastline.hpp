#pragma once

#include "generator/coastlines_generator.hpp"
#include "generator/final_processor_interface.hpp"

#include <string>

namespace generator
{
class CoastlineFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  explicit CoastlineFinalProcessor(std::string const & filename);

  void SetCoastlinesFilenames(std::string const & geomFilename,
                              std::string const & rawGeomFilename);

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

private:
  std::string m_filename;
  std::string m_coastlineGeomFilename;
  std::string m_coastlineRawGeomFilename;
  CoastlineFeaturesGenerator m_generator;
};
}  // namespace generator
