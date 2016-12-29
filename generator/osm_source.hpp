#pragma once

#include "generator/generate_info.hpp"
#include "generator/osm_element.hpp"

#include "std/function.hpp"
#include "std/iostream.hpp"
#include "std/string.hpp"
#include "std/sstream.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class SourceReader
{
  struct Deleter
  {
    bool m_needDelete;
    Deleter(bool needDelete = true) : m_needDelete(needDelete) {}
    void operator()(istream * s) const
    {
      if (m_needDelete)
        delete s;
    }
  };

  unique_ptr<istream, Deleter> m_file;

public:
  SourceReader();
  explicit SourceReader(string const & filename);
  explicit SourceReader(istringstream & stream);

  uint64_t Read(char * buffer, uint64_t bufferSize);
};

class FeatureBuilder1;

// Emitter is used in OsmElemen to FeatureBuilder translation process.
class EmitterBase
{
public:
  virtual ~EmitterBase() = default;

  /// This method is used by OsmTranslator to pass |fb| to Emitter for further processing.
  virtual void operator()(FeatureBuilder1 & fb) = 0;
  /// Finish is used in GenerateFeatureImpl to make whatever work is needed after
  /// all OmsElements are processed.
  virtual bool Finish() { return true; }
  /// Sets buckets (mwm names).
  // TODO(syershov): Make this topic clear.
  virtual void GetNames(vector<string> & names) const = 0;
};

unique_ptr<EmitterBase> MakeMainFeatureEmitter(feature::GenerateInfo const & info);

using EmitterFactory = function<unique_ptr<EmitterBase>(feature::GenerateInfo const &)>;

bool GenerateFeatures(feature::GenerateInfo & info,
                      EmitterFactory factory = MakeMainFeatureEmitter);
bool GenerateIntermediateData(feature::GenerateInfo & info);

void ProcessOsmElementsFromO5M(SourceReader & stream, function<void(OsmElement *)> processor);
void ProcessOsmElementsFromXML(SourceReader & stream, function<void(OsmElement *)> processor);
