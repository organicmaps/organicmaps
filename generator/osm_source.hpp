#pragma once

#include "generator/generate_info.hpp"
#include "generator/osm_element.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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

class FeatureBuilder1;
class FeatureParams;

// Emitter is used in OsmElemen to FeatureBuilder translation process.
class EmitterBase
{
public:
  virtual ~EmitterBase() = default;

  /// This method is used by OsmTranslator to pass |fb| to Emitter for further processing.
  virtual void operator()(FeatureBuilder1 & fb) = 0;

  virtual void EmitCityBoundary(FeatureBuilder1 const & fb, FeatureParams const & params) {}

  /// Finish is used in GenerateFeatureImpl to make whatever work is needed after
  /// all OmsElements are processed.
  virtual bool Finish() { return true; }
  /// Sets buckets (mwm names).
  // TODO(syershov): Make this topic clear.
  virtual void GetNames(std::vector<std::string> & names) const = 0;
};

std::unique_ptr<EmitterBase> MakeMainFeatureEmitter(feature::GenerateInfo const & info);

using EmitterFactory = std::function<std::unique_ptr<EmitterBase>(feature::GenerateInfo const &)>;

bool GenerateFeatures(feature::GenerateInfo & info,
                      EmitterFactory factory = MakeMainFeatureEmitter);
bool GenerateIntermediateData(feature::GenerateInfo & info);

void ProcessOsmElementsFromO5M(SourceReader & stream, std::function<void(OsmElement *)> processor);
void ProcessOsmElementsFromXML(SourceReader & stream, std::function<void(OsmElement *)> processor);
