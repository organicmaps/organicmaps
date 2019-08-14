#pragma once

#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_o5m_source.hpp"
#include "generator/osm_xml_source.hpp"
#include "generator/translator_interface.hpp"

#include "coding/parse_xml.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>

struct OsmElement;
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

class LoaderWrapper
{
public:
  LoaderWrapper(feature::GenerateInfo & info);
  cache::IntermediateDataReader & GetReader();

private:
  std::unique_ptr<cache::IntermediateData> m_reader;
};

class CacheLoader
{
public:
  CacheLoader(feature::GenerateInfo & info);
  cache::IntermediateDataReader & GetCache();

private:
  feature::GenerateInfo & m_info;
  std::unique_ptr<LoaderWrapper> m_loader;

  DISALLOW_COPY(CacheLoader);
};
// This function is needed to generate intermediate data from OSM elements.
// The translators collection contains translators that translate the OSM element into
// some intermediate representation.
//
// To better understand the generation process of this step,
// we are looking at generation using the example of generation for countries.
//
// To start the generation we make the following calls:
// 1. feature::GenerateInfo genInfo;
// ...
// 2. CacheLoader cacheLoader(genInfo);
// 3. TranslatorCollection translators;
// 4. auto emitter = CreateEmitter(EmitterType::Country, genInfo);
// 5. translators.Append(CreateTranslator(TranslatorType::Country, emitter, cacheLoader.GetCache(),
// genInfo));
// 6. GenerateRaw(genInfo, translators);
//
// In line 5, we create and add a translator for countries to the translator collection.
// TranslatorCountry is inheritor of Translator.
//
// Translator contains several important entities: FeatureMaker, FilterCollection,
// CollectorCollection and Emitter. In short,
// * FeatureMaker - an object that can create FeatureBuilder1 from OSM element,
// * FilterCollection - an object that contains a group of filters that may or may not pass OSM
// elements and FeatureBuilder1s,
// * CollectorCollection - an object that contains a group of collectors that collect additional
// information about OSM elements and FeatureBuilder1s (most often it is information that cannot
// be saved in FeatureBuilder1s from OSM element),
// * Emitter - an object that converts an element and saves it.
//
// The most important method is Translator::Emit. Please read it to understand how generation works.
// The order of calls is very important. First, the FilterCollection will filter the OSM elements,
// then call the CollectorCollection for OSM elements, then build the FeatureBuilder1 element
// form OSM element, then FilterCollection will filter the FeatureBuilder1, then call the
// CollectorCollection for the FeatureBuilder1, and then FeatureBuilder1 will fall into the emitter.
//
// TranslatorCountry contains for it specific filters, collectors, emitter and FeatureMaker.
// For example, there are FilterPlanet, which only passes relations with types multipolygon or
// boundary, and CameraNodeProcessor, which collects information about the cameras on the roads.
//
// In line 4, we create emitter for countries.
// The emitter is an important entity that needs to transform FeatureBuilder1 and save them in some
// way. The emitter can filter objects and change the representation of an object based on drawing
// rules and other application rules. In EmitterCountry stages are divided into layers. The layers
// are connected in a chain. For example, there are RepresentationLayer, which may change the
// presentation of the FeatureBuilder1 depending on the rules of the application, and BookingLayer,
// which mixes information from booking. You can read a more detailed look into the appropriate
// class code.
bool GenerateRaw(feature::GenerateInfo & info, TranslatorInterface & translators);
bool GenerateIntermediateData(feature::GenerateInfo & info);

void ProcessOsmElementsFromO5M(SourceReader & stream, std::function<void(OsmElement *)> processor);
void ProcessOsmElementsFromXML(SourceReader & stream, std::function<void(OsmElement *)> processor);

class ProcessorOsmElementsInterface
{
public:
  virtual ~ProcessorOsmElementsInterface() = default;

  virtual bool TryRead(OsmElement & element) = 0;
};

class ProcessorOsmElementsFromO5M : public ProcessorOsmElementsInterface
{
public:
  explicit ProcessorOsmElementsFromO5M(SourceReader & stream);

  // ProcessorOsmElementsInterface overrides:
  bool TryRead(OsmElement & element) override;

private:
  SourceReader & m_stream;
  osm::O5MSource m_dataset;
  osm::O5MSource::Iterator m_pos;
};

class ProcessorOsmElementsFromXml : public ProcessorOsmElementsInterface
{
public:
  explicit ProcessorOsmElementsFromXml(SourceReader & stream);

  // ProcessorOsmElementsInterface overrides:
  bool TryRead(OsmElement & element) override;

private:
  bool TryReadFromQueue(OsmElement & element);

  XMLSource m_xmlSource;
  XMLSequenceParser<SourceReader, XMLSource> m_parser;
  std::queue<OsmElement> m_queue;
};
}  // namespace generator
