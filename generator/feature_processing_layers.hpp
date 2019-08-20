#pragma once

#include "generator/affiliation.hpp"
#include "generator/booking_dataset.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/features_processing_helpers.hpp"
#include "generator/filter_world.hpp"
#include "generator/opentable_dataset.hpp"
#include "generator/processor_interface.hpp"
#include "generator/promo_catalog_cities.hpp"
#include "generator/world_map_generator.hpp"

#include <memory>
#include <sstream>
#include <string>

class CoastlineFeaturesGenerator;

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
class PlaceProcessor;
// Responsibility of the class Log Buffer - encapsulation of the buffer for internal logs.
class LogBuffer
{
public:
  template <class T, class... Ts>
  void AppendLine(T const & value, Ts... rest)
  {
    AppendImpl(value, rest...);
    // The last "\t" is overwritten here
    m_buffer.seekp(-1, std::ios_base::end);
    m_buffer << "\n";
  }

  std::string GetAsString() const;

private:
  template <class T, class... Ts>
  void AppendImpl(T const & value, Ts... rest)
  {
    m_buffer << value << "\t";
    AppendImpl(rest...);
  }

  void AppendImpl() {}

  std::ostringstream m_buffer;
};

// This is the base layer class. Inheriting from it allows you to create a chain of layers.
class LayerBase : public std::enable_shared_from_this<LayerBase>
{
public:
  LayerBase() = default;
  virtual ~LayerBase() = default;

  // The function works in linear time from the number of layers that exist after that.
  virtual void Handle(feature::FeatureBuilder & fb);

  void Merge(std::shared_ptr<LayerBase> const & other);
  void MergeChain(std::shared_ptr<LayerBase> const & other);

  size_t GetChainSize() const;

  void SetNext(std::shared_ptr<LayerBase> next);
  std::shared_ptr<LayerBase> Add(std::shared_ptr<LayerBase> next);

  template <class T, class... Ts>
  constexpr void AppendLine(T const & value, Ts... rest)
  {
    m_logBuffer.AppendLine(value, rest...);
  }

  std::string GetAsString() const;
  std::string GetAsStringRecursive() const;

private:
  LogBuffer m_logBuffer;
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
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;

private:
  static bool CanBeArea(FeatureParams const & params);
  static bool CanBePoint(FeatureParams const & params);
  static bool CanBeLine(FeatureParams const & params);

  void HandleArea(feature::FeatureBuilder & fb, FeatureParams const & params);
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

class PreserializeLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override;
};

template <class SerializePolicy = feature::serialization_policy::MaxAccuracy>
class AffiliationsFeatureLayer : public LayerBase
{
public:
  AffiliationsFeatureLayer(size_t bufferSize, std::shared_ptr<feature::AffiliationInterface> const & affiliation,
                          std::shared_ptr<FeatureProcessorQueue> const & queue)
    : m_bufferSize(bufferSize)
    , m_affiliation(affiliation)
    , m_queue(queue)
  {
    m_buffer.reserve(m_bufferSize);
  }

  // LayerBase overrides:
  void Handle(feature::FeatureBuilder & fb) override
  {
    feature::FeatureBuilder::Buffer buffer;
    SerializePolicy::Serialize(fb, buffer);
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
  std::shared_ptr<feature::AffiliationInterface> m_affiliation;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
};
}  // namespace generator
