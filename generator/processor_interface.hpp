#pragma once

#include "generator/feature_builder.hpp"

#include "base/assert.hpp"
#include "base/thread_safe_queue.hpp"

#include <memory>
#include <string>
#include <vector>

class FeatureParams;

namespace generator
{
class ProcessorCoastline;
class ProcessorCountry;
class ProcessorNoop;
class ProcessorSimple;
class ProcessorWorld;

// Implementing this interface allows an object to process FeatureBuilder objects and broadcast them.
class FeatureProcessorInterface
{
public:
  virtual ~FeatureProcessorInterface() = default;

  virtual std::shared_ptr<FeatureProcessorInterface> Clone() const = 0;

  // This method is used by OsmTranslator to pass |fb| to Processor for further processing.
  virtual void Process(feature::FeatureBuilder & fb) = 0;
  virtual void Flush() = 0;

  virtual void Merge(FeatureProcessorInterface const &) = 0;

  virtual void MergeInto(ProcessorCoastline &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(ProcessorCountry &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(ProcessorNoop &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(ProcessorSimple &) const { FailIfMethodUnsupported(); }
  virtual void MergeInto(ProcessorWorld &) const { FailIfMethodUnsupported(); }

private:
  void FailIfMethodUnsupported() const { CHECK(false, ("This method is unsupported.")); }
};

size_t static const kAffilationsBufferSize = 128;

struct ProcessedData
{
  explicit ProcessedData(feature::FeatureBuilder::Buffer && buffer, std::vector<std::string>  && affiliations)
    : m_buffer(std::move(buffer)), m_affiliations(std::move(affiliations))
  {
  }

  feature::FeatureBuilder::Buffer m_buffer;
  std::vector<std::string> m_affiliations;
};

using FeatureProcessorChank = base::threads::DataWrapper<std::vector<ProcessedData>>;
using FeatureProcessorQueue = base::threads::ThreadSafeQueue<FeatureProcessorChank>;

}  // namespace generator
