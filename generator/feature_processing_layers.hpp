#pragma once

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"
#include "generator/features_processing_helpers.hpp"
#include "generator/filter_world.hpp"
#include "generator/processor_interface.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace generator
{

// This is the base layer class. Inheriting from it allows you to create a chain of layers.
class LayerBase : public std::enable_shared_from_this<LayerBase>
{
public:
  LayerBase() = default;
  virtual ~LayerBase() = default;

  // The function works in linear time from the number of layers that exist after that.
  virtual void Handle(feature::FeatureBuilder & fb);

  size_t GetChainSize() const;
  void Add(std::shared_ptr<LayerBase> next);

private:
  std::shared_ptr<LayerBase> m_next;
};

// Responsibility of class RepresentationLayer is converting features from one form to another for countries.
// Here we can use the knowledge of the rules for drawing objects.
// Osm object can be represented as feature of following geometry types: point, line, area depending on
// its types and geometry. Sometimes one osm object can be represented as two features e.g. object with
// closed geometry with types "leisure=playground" and "barrier=fence" splits into two objects: area object
// with type "leisure=playground" and line object with type "barrier=fence".
class RepresentationLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;

  static bool CanBeArea(FeatureParams const & params);
  static bool CanBePoint(FeatureParams const & params);
  static bool CanBeLine(FeatureParams const & params);

private:
  void HandleArea(feature::FeatureBuilder & fb, FeatureBuilderParams const & params);

  // std::shared_ptr<ComplexFeaturesMixer> m_complexFeaturesMixer;
};

// Responsibility of class PrepareFeatureLayer is the removal of unused types and names,
// as well as the preparation of features for further processing for countries.
class PrepareFeatureLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;
};

// Responsibility of class RepresentationCoastlineLayer is converting features from one form to
// another for coastlines. Here we can use the knowledge of the rules for drawing objects.
class RepresentationCoastlineLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;
};

// Responsibility of class PrepareCoastlineFeatureLayer is the removal of unused types and names,
// as well as the preparation of features for further processing for coastlines.
class PrepareCoastlineFeatureLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;
};

class WorldLayer : public LayerBase
{
public:
  explicit WorldLayer(std::string const & popularityFilename);

  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;

private:
  FilterWorld m_filter;
};

class CountryLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;
};

/*
class PreserializeLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;
};
*/

class AffiliationsFeatureLayer : public LayerBase
{
public:
  AffiliationsFeatureLayer(size_t bufferSize, AffiliationInterfacePtr affiliation,
                           std::shared_ptr<FeatureProcessorQueue> queue)
    : m_bufferSize(bufferSize)
    , m_affiliation(std::move(affiliation))
    , m_queue(std::move(queue))
  {
    m_buffer.reserve(m_bufferSize);
  }

  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override
  {
    feature::FeatureBuilder::Buffer buffer;
    feature::serialization_policy::MaxAccuracy::Serialize(fb, buffer);
    m_buffer.emplace_back(std::move(buffer), m_affiliation->GetAffiliations(fb));
    if (m_buffer.size() >= m_bufferSize)
      AddBufferToQueue();
  }

  bool AddBufferToQueue()
  {
    if (m_buffer.empty())
      return false;

    m_queue->Push(std::move(m_buffer));
    m_buffer.clear();
    m_buffer.reserve(m_bufferSize);
    return true;
  }

private:
  size_t const m_bufferSize;
  std::vector<ProcessedData> m_buffer;
  AffiliationInterfacePtr m_affiliation;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
};
/*
class ComplexFeaturesMixer
{
public:
  explicit ComplexFeaturesMixer(std::unordered_set<CompositeId> const & hierarchyNodesSet);

  void Process(std::function<void(feature::FeatureBuilder &)> next,
               feature::FeatureBuilder const & fb);

  std::shared_ptr<ComplexFeaturesMixer> Clone();

private:
  feature::FeatureBuilder MakeComplexLineFrom(feature::FeatureBuilder const & fb);
  feature::FeatureBuilder MakeComplexAreaFrom(feature::FeatureBuilder const & fb);

  std::unordered_set<CompositeId> const & m_hierarchyNodesSet;
  uint32_t const m_complexEntryType;
};
*/
}  // namespace generator
