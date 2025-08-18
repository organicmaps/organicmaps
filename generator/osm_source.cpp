#include "generator/osm_source.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"
#include "generator/towns_dumper.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <fstream>
#include <memory>

#include "defines.hpp"

namespace generator
{
// SourceReader ------------------------------------------------------------------------------------
SourceReader::SourceReader() : m_file(std::unique_ptr<std::istream, Deleter>(&std::cin, Deleter(false)))
{
  LOG_SHORT(LINFO, ("Reading OSM data from stdin"));
}

SourceReader::SourceReader(std::string const & filename)
  : m_file(std::unique_ptr<std::istream, Deleter>(new std::ifstream(filename), Deleter()))
{
  CHECK(static_cast<std::ifstream *>(m_file.get())->is_open(), ("Can't open file:", filename));
  LOG_SHORT(LINFO, ("Reading OSM data from", filename));
}

SourceReader::SourceReader(std::istringstream & stream)
  : m_file(std::unique_ptr<std::istream, Deleter>(&stream, Deleter(false)))
{
  LOG_SHORT(LINFO, ("Reading OSM data from memory"));
}

uint64_t SourceReader::Read(char * buffer, uint64_t bufferSize)
{
  m_file->read(buffer, static_cast<std::streamsize>(bufferSize));
  auto const gcount = static_cast<uint64_t>(m_file->gcount());
  m_pos += gcount;
  return gcount;
}

// Functions ---------------------------------------------------------------------------------------
void AddElementToCache(cache::IntermediateDataWriter & cache, OsmElement && element)
{
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node:
  {
    auto const pt = mercator::FromLatLon(element.m_lat, element.m_lon);
    cache.AddNode(element.m_id, pt.y, pt.x);
    break;
  }
  case OsmElement::EntityType::Way:
  {
    // Store way.
    WayElement way(element.m_id);
    way.m_nodes = std::move(element.NodesRef());

    if (way.IsValid())
      cache.AddWay(element.m_id, way);
    break;
  }
  case OsmElement::EntityType::Relation:
  {
    // store relation
    RelationElement relation;
    for (auto & member : element.MembersRef())
    {
      switch (member.m_type)
      {
      case OsmElement::EntityType::Node: relation.m_nodes.emplace_back(member.m_ref, std::move(member.m_role)); break;
      case OsmElement::EntityType::Way: relation.m_ways.emplace_back(member.m_ref, std::move(member.m_role)); break;
      case OsmElement::EntityType::Relation:
        relation.m_relations.emplace_back(member.m_ref, std::move(member.m_role));
        break;
      default: break;
      }
    }

    for (auto & tag : element.TagsRef())
      relation.m_tags.emplace(std::move(tag.m_key), std::move(tag.m_value));

    if (relation.IsValid())
      cache.AddRelation(element.m_id, relation);

    break;
  }
  default: break;
  }
}

void ProcessOsmElementsFromXML(SourceReader & stream, std::function<void(OsmElement &&)> const & processor)
{
  ProcessorOsmElementsFromXml processorOsmElementsFromXml(stream);
  OsmElement element;
  while (processorOsmElementsFromXml.TryRead(element))
  {
    processor(std::move(element));
    // It is safe to use `element` here as `Clear` will restore the state after the move.
    element.Clear();
  }
}

void ProcessOsmElementsFromO5M(SourceReader & stream, std::function<void(OsmElement &&)> const & processor)
{
  ProcessorOsmElementsFromO5M processorOsmElementsFromO5M(stream);
  OsmElement element;
  while (processorOsmElementsFromO5M.TryRead(element))
  {
    processor(std::move(element));
    // It is safe to use `element` here as `Clear` will restore the state after the move.
    element.Clear();
  }
}

ProcessorOsmElementsFromO5M::ProcessorOsmElementsFromO5M(SourceReader & stream)
  : m_stream(stream)
  , m_dataset([&](uint8_t * buffer, size_t size) { return m_stream.Read(reinterpret_cast<char *>(buffer), size); })
  , m_pos(m_dataset.begin())
{}

bool ProcessorOsmElementsFromO5M::TryRead(OsmElement & element)
{
  if (m_pos == m_dataset.end())
    return false;

  using Type = osm::O5MSource::EntityType;
  auto const translate = [](Type t) -> OsmElement::EntityType
  {
    switch (t)
    {
    case Type::Node: return OsmElement::EntityType::Node;
    case Type::Way: return OsmElement::EntityType::Way;
    case Type::Relation: return OsmElement::EntityType::Relation;
    default: return OsmElement::EntityType::Unknown;
    }
  };

  // Be careful, we could call Nodes(), Members(), Tags() from O5MSource::Entity
  // only once (!). Because these functions read data from file simultaneously with
  // iterating in loop. Furthermore, into Tags() method calls Nodes.Skip() and Members.Skip(),
  // thus first call of Nodes (Members) after Tags() will not return any results.
  // So don not reorder the "for" loops (!).
  auto const & entity = *m_pos;
  element.m_id = entity.id;
  switch (entity.type)
  {
  case Type::Node:
  {
    element.m_type = OsmElement::EntityType::Node;
    element.m_lat = entity.lat;
    element.m_lon = entity.lon;
    break;
  }
  case Type::Way:
  {
    element.m_type = OsmElement::EntityType::Way;
    for (uint64_t nd : entity.Nodes())
      element.AddNd(nd);
    break;
  }
  case Type::Relation:
  {
    element.m_type = OsmElement::EntityType::Relation;
    for (auto const & member : entity.Members())
      element.AddMember(member.ref, translate(member.type), member.role);
    break;
  }
  default: break;
  }

  for (auto const & tag : entity.Tags())
    element.AddTag(tag.key, tag.value);

  element.Validate();
  ++m_pos;
  return true;
}

ProcessorOsmElementsFromXml::ProcessorOsmElementsFromXml(SourceReader & stream)
  : m_xmlSource([&, this](OsmElement && e) { m_queue.emplace(std::move(e)); })
  , m_parser(stream, m_xmlSource)
{}

bool ProcessorOsmElementsFromXml::TryReadFromQueue(OsmElement & element)
{
  if (m_queue.empty())
    return false;

  element = m_queue.front();
  m_queue.pop();

  element.Validate();
  return true;
}

bool ProcessorOsmElementsFromXml::TryRead(OsmElement & element)
{
  do
  {
    if (TryReadFromQueue(element))
      return true;
  }
  while (m_parser.Read());

  return TryReadFromQueue(element);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////

bool GenerateIntermediateData(feature::GenerateInfo & info)
{
  auto nodes = cache::CreatePointStorageWriter(info.m_nodeStorageType, info.GetCacheFileName(NODES_FILE));
  cache::IntermediateDataWriter cache(*nodes, info);
  TownsDumper towns;
  SourceReader reader = info.m_osmFileName.empty() ? SourceReader() : SourceReader(info.m_osmFileName);

  LOG(LINFO, ("Data source:", info.m_osmFileName));

  auto const processor = [&](OsmElement && element)
  {
    towns.CheckElement(element);
    AddElementToCache(cache, std::move(element));
  };

  switch (info.m_osmFileType)
  {
  case feature::GenerateInfo::OsmSourceType::XML: ProcessOsmElementsFromXML(reader, processor); break;
  case feature::GenerateInfo::OsmSourceType::O5M: ProcessOsmElementsFromO5M(reader, processor); break;
  }

  cache.SaveIndex();
  towns.Dump(info.GetIntermediateFileName(TOWNS_FILE));
  LOG(LINFO, ("Added points count =", nodes->GetNumProcessedPoints()));
  return true;
}
}  // namespace generator
