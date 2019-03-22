#pragma once

#include "generator/booking_dataset.hpp"
#include "generator/feature_generator.hpp"
#include "generator/opentable_dataset.hpp"
#include "generator/polygonizer.hpp"
#include "generator/world_map_generator.hpp"

#include <memory>
#include <sstream>
#include <string>

class FeatureBuilder1;
class CoastlineFeaturesGenerator;

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
class CityBoundaryProcessor;
class CountryMapper;
class WorldMapper;

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
  virtual void Handle(FeatureBuilder1 & feature);

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
// The class transfers control to the CityBoundaryProcessor if for the feature it is a city, town or village.
// Osm object can be represented as feature of following geometry types: point, line, area depending on
// its types and geometry. Sometimes one osm object can be represented as two features e.g. object with
// closed geometry with types "leisure=playground" and "barrier=fence" splits into two objects: area object
// with type "leisure=playground" and line object with type "barrier=fence".
class RepresentationLayer : public LayerBase
{
public:
  explicit RepresentationLayer(std::shared_ptr<CityBoundaryProcessor> processor);

  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;

private:
  static bool CanBeArea(FeatureParams const & params);
  static bool CanBePoint(FeatureParams const & params);
  static bool CanBeLine(FeatureParams const & params);

  void HandleArea(FeatureBuilder1 & feature, FeatureParams const & params);

  std::shared_ptr<CityBoundaryProcessor> m_processor;
};

// Responsibility of class PrepareFeatureLayer is the removal of unused types and names,
// as well as the preparation of features for further processing for countries.
class PrepareFeatureLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;
};

// Responsibility of class CityBoundaryLayer - transfering control to the CityBoundaryProcessor
// if the feature is a place.
class CityBoundaryLayer : public LayerBase
{
public:
  explicit CityBoundaryLayer(std::shared_ptr<CityBoundaryProcessor> processor);

  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;

private:
  std::shared_ptr<CityBoundaryProcessor> m_processor;
};

// Responsibility of class BookingLayer - mixing information from booking. If there is a
// coincidence of the hotel and feature, the processing of the feature is performed.
class BookingLayer : public LayerBase
{
public:
  explicit BookingLayer(std::string const & filename, std::shared_ptr<CountryMapper> countryMapper);

  // LayerBase overrides:
  ~BookingLayer() override;

  void Handle(FeatureBuilder1 & feature) override;

private:
  BookingDataset m_dataset;
  std::shared_ptr<CountryMapper> m_countryMapper;
};

// Responsibility of class OpentableLayer - mixing information from opentable. If there is a
// coincidence of the restaurant and feature, the processing of the feature is performed.
class OpentableLayer : public LayerBase
{
public:
  explicit OpentableLayer(std::string const & filename, std::shared_ptr<CountryMapper> countryMapper);

  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;

private:
  OpentableDataset m_dataset;
  std::shared_ptr<CountryMapper> m_countryMapper;
};

// Responsibility of class CountryMapperLayer - mapping of features to countries.
class CountryMapperLayer : public LayerBase
{
public:
  explicit CountryMapperLayer(std::shared_ptr<CountryMapper> countryMapper);

  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;

private:
  std::shared_ptr<CountryMapper> m_countryMapper;
};

// Responsibility of class EmitCoastsLayer is adding coastlines to countries.
class EmitCoastsLayer : public LayerBase
{
public:
  explicit EmitCoastsLayer(std::string const & worldCoastsFilename, std::string const & geometryFilename,
                           std::shared_ptr<CountryMapper> countryMapper);

  // LayerBase overrides:
  ~EmitCoastsLayer() override;

  void Handle(FeatureBuilder1 & feature) override;

private:
  std::shared_ptr<feature::FeaturesCollector> m_collector;
  std::shared_ptr<CountryMapper> m_countryMapper;
  std::string m_geometryFilename;
};

// Responsibility of class RepresentationCoastlineLayer is converting features from one form to
// another for coastlines. Here we can use the knowledge of the rules for drawing objects.
class RepresentationCoastlineLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;
};

// Responsibility of class PrepareCoastlineFeatureLayer is the removal of unused types and names,
// as well as the preparation of features for further processing for coastlines.
class PrepareCoastlineFeatureLayer : public LayerBase
{
public:
  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;
};

// Responsibility of class CoastlineMapperLayer - mapping of features on coastline.
class CoastlineMapperLayer : public LayerBase
{
public:
  explicit CoastlineMapperLayer(std::shared_ptr<CoastlineFeaturesGenerator> coastlineMapper);

  // LayerBase overrides:
  void Handle(FeatureBuilder1 & feature) override;

private:
  std::shared_ptr<CoastlineFeaturesGenerator> m_coastlineGenerator;
};

// Responsibility of class CountryMapperLayer - mapping of features on the world.
class WorldAreaLayer : public LayerBase
{
public:
  using WorldGenerator = WorldMapGenerator<feature::FeaturesCollector>;

  explicit WorldAreaLayer(std::shared_ptr<WorldMapper> mapper);

  // LayerBase overrides:
  ~WorldAreaLayer() override;

  void Handle(FeatureBuilder1 & feature) override;

private:
  std::shared_ptr<WorldMapper> m_mapper;
};

// This is the class-wrapper over CountriesGenerator class.
class CountryMapper
{
public:
  using Polygonizer = feature::Polygonizer<feature::FeaturesCollector>;
  using CountriesGenerator = CountryMapGenerator<Polygonizer>;

  explicit CountryMapper(feature::GenerateInfo const & info);

  void Map(FeatureBuilder1 & feature);
  void RemoveInvalidTypesAndMap(FeatureBuilder1 & feature);
  Polygonizer & Parent();
  std::vector<std::string> const & GetNames() const;

private:
  std::unique_ptr<CountriesGenerator> m_countries;
};

// This is the class-wrapper over WorldGenerator class.
class WorldMapper
{
public:
  using WorldGenerator = WorldMapGenerator<feature::FeaturesCollector>;

  explicit WorldMapper(std::string const & worldFilename, std::string const & rawGeometryFilename,
                       std::string const & popularPlacesFilename);

  void Map(FeatureBuilder1 & feature);
  void RemoveInvalidTypesAndMap(FeatureBuilder1 & feature);
  void Merge();

private:
  std::unique_ptr<WorldGenerator> m_world;
};
}  // namespace generator
