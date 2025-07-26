#pragma once

#include "base/assert.hpp"

#include <memory>
#include <string>
#include <vector>

struct OsmElement;

namespace generator
{
namespace cache
{
class IntermediateData;
}  // namespace cache

class TranslatorCountry;
class TranslatorCoastline;
class TranslatorWorld;
class TranslatorCollection;
class TranslatorComplex;

// Implementing this interface allows an object to create intermediate data from OsmElement.
class TranslatorInterface
{
public:
  virtual ~TranslatorInterface() = default;

  virtual std::shared_ptr<TranslatorInterface> Clone() const = 0;

  virtual void Preprocess(OsmElement &) {}
  virtual void Emit(OsmElement const & element) = 0;
  virtual void Finish() = 0;
  virtual bool Save() = 0;

  virtual void Merge(TranslatorInterface const &) = 0;
};
}  // namespace generator

#define IMPLEMENT_TRANSLATOR_IFACE(className)             \
  void Merge(TranslatorInterface const & ti) override     \
  {                                                       \
    dynamic_cast<className const &>(ti).MergeInto(*this); \
  }
