#pragma once

#include "generator/emitter_interface.hpp"
#include "generator/generate_info.hpp"
#include "generator/osm_element.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class FeatureBuilder1;
class FeatureParams;

namespace generator
{
class SourceReader
{
  struct Deleter
  {
    bool m_needDelete;
    Deleter(bool needDelete = true) : m_needDelete(needDelete) {}
    void operator()(std::istream * s) const
    {
      if (m_needDelete)
        delete s;
    }
  };

  std::unique_ptr<std::istream, Deleter> m_file;

public:
  SourceReader();
  explicit SourceReader(std::string const & filename);
  explicit SourceReader(std::istringstream & stream);

  uint64_t Read(char * buffer, uint64_t bufferSize);
};

bool GenerateFeatures(feature::GenerateInfo & info, std::shared_ptr<EmitterInterface> emitter);
bool GenerateRegionFeatures(feature::GenerateInfo & info, std::shared_ptr<EmitterInterface> emitter);
bool GenerateIntermediateData(feature::GenerateInfo & info);

void ProcessOsmElementsFromO5M(SourceReader & stream, std::function<void(OsmElement *)> processor);
void ProcessOsmElementsFromXML(SourceReader & stream, std::function<void(OsmElement *)> processor);
}  // namespace generator
