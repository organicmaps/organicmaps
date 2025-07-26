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
  uint64_t m_pos = 0;

public:
  SourceReader();
  explicit SourceReader(std::string const & filename);
  explicit SourceReader(std::istringstream & stream);

  uint64_t Read(char * buffer, uint64_t bufferSize);
  uint64_t Pos() const { return m_pos; }
};

bool GenerateIntermediateData(feature::GenerateInfo & info);

void ProcessOsmElementsFromO5M(SourceReader & stream, std::function<void(OsmElement &&)> const & processor);
void ProcessOsmElementsFromXML(SourceReader & stream, std::function<void(OsmElement &&)> const & processor);

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
