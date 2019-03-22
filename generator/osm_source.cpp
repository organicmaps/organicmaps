#include "generator/osm_source.hpp"

#include "generator/camera_node_processor.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/node_mixer.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_o5m_source.hpp"
#include "generator/osm_xml_source.hpp"
#include "generator/towns_dumper.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"
#include "geometry/tree4d.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/parse_xml.hpp"

#include <memory>
#include <set>

#include "defines.hpp"

using namespace std;

namespace generator
{
// SourceReader ------------------------------------------------------------------------------------
SourceReader::SourceReader() : m_file(unique_ptr<istream, Deleter>(&cin, Deleter(false)))
{
  LOG_SHORT(LINFO, ("Reading OSM data from stdin"));
}

SourceReader::SourceReader(string const & filename)
  : m_file(unique_ptr<istream, Deleter>(new ifstream(filename), Deleter()))
{
  CHECK(static_cast<ifstream *>(m_file.get())->is_open(), ("Can't open file:", filename));
  LOG_SHORT(LINFO, ("Reading OSM data from", filename));
}

SourceReader::SourceReader(istringstream & stream)
  : m_file(unique_ptr<istream, Deleter>(&stream, Deleter(false)))
{
  LOG_SHORT(LINFO, ("Reading OSM data from memory"));
}

uint64_t SourceReader::Read(char * buffer, uint64_t bufferSize)
{
  m_file->read(buffer, bufferSize);
  return m_file->gcount();
}

// Functions ---------------------------------------------------------------------------------------
void AddElementToCache(cache::IntermediateDataWriter & cache,
                       CameraNodeIntermediateDataProcessor & cameras, OsmElement & em)
{
  switch (em.type)
  {
  case OsmElement::EntityType::Node:
  {
    auto const pt = MercatorBounds::FromLatLon(em.lat, em.lon);
    cache.AddNode(em.id, pt.y, pt.x);
    cameras.ProcessNode(em);
    break;
  }
  case OsmElement::EntityType::Way:
  {
    // Store way.
    WayElement way(em.id);
    for (uint64_t nd : em.Nodes())
      way.nodes.push_back(nd);

    if (way.IsValid())
    {
      cache.AddWay(em.id, way);
      cameras.ProcessWay(em.id, way);
    }
    break;
  }
  case OsmElement::EntityType::Relation:
  {
    // store relation
    RelationElement relation;
    for (auto const & member : em.Members())
    {
      switch (member.type) {
      case OsmElement::EntityType::Node:
        relation.nodes.emplace_back(member.ref, string(member.role));
        break;
      case OsmElement::EntityType::Way:
        relation.ways.emplace_back(member.ref, string(member.role));
        break;
      case OsmElement::EntityType::Relation:
        // we just ignore type == "relation"
        break;
      default:
        break;
      }
    }

    for (auto const & tag : em.Tags())
      relation.tags.emplace(tag.key, tag.value);

    if (relation.IsValid())
      cache.AddRelation(em.id, relation);

    break;
  }
  default:
    break;
  }
}

void BuildIntermediateDataFromXML(SourceReader & stream, cache::IntermediateDataWriter & cache,
                                  TownsDumper & towns, CameraNodeIntermediateDataProcessor & cameras)
{
  XMLSource parser([&](OsmElement * e)
  {
    towns.CheckElement(*e);
    AddElementToCache(cache, cameras, *e);
  });
  ParseXMLSequence(stream, parser);
}

void ProcessOsmElementsFromXML(SourceReader & stream, function<void(OsmElement *)> processor)
{
  XMLSource parser([&](OsmElement * e) { processor(e); });
  ParseXMLSequence(stream, parser);
}

void BuildIntermediateDataFromO5M(SourceReader & stream, cache::IntermediateDataWriter & cache,
                                  TownsDumper & towns, CameraNodeIntermediateDataProcessor & cameras)
{
  auto processor = [&cache, &cameras, &towns](OsmElement * em) {
    towns.CheckElement(*em);
    AddElementToCache(cache, cameras, *em);
  };

  // Use only this function here, look into ProcessOsmElementsFromO5M
  // for more details.
  ProcessOsmElementsFromO5M(stream, processor);
}

void ProcessOsmElementsFromO5M(SourceReader & stream, function<void(OsmElement *)> processor)
{
  using Type = osm::O5MSource::EntityType;

  osm::O5MSource dataset([&stream](uint8_t * buffer, size_t size)
  {
    return stream.Read(reinterpret_cast<char *>(buffer), size);
  });

  auto translate = [](Type t) -> OsmElement::EntityType {
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

  for (auto const & em : dataset)
  {
    OsmElement p;
    p.id = em.id;

    switch (em.type)
    {
    case Type::Node:
    {
      p.type = OsmElement::EntityType::Node;
      p.lat = em.lat;
      p.lon = em.lon;
      break;
    }
    case Type::Way:
    {
      p.type = OsmElement::EntityType::Way;
      for (uint64_t nd : em.Nodes())
        p.AddNd(nd);
      break;
    }
    case Type::Relation:
    {
      p.type = OsmElement::EntityType::Relation;
      for (auto const & member : em.Members())
        p.AddMember(member.ref, translate(member.type), member.role);
      break;
    }
    default: break;
    }

    for (auto const & tag : em.Tags())
      p.AddTag(tag.key, tag.value);

    processor(&p);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////

bool GenerateRaw(feature::GenerateInfo & info, TranslatorInterface & translators)
{
  auto const fn = [&](OsmElement * e) {
    CHECK(e, ());
    auto & element = *e;
    translators.Emit(element);
  };

  SourceReader reader = info.m_osmFileName.empty() ? SourceReader() : SourceReader(info.m_osmFileName);
  switch (info.m_osmFileType)
  {
  case feature::GenerateInfo::OsmSourceType::XML:
    ProcessOsmElementsFromXML(reader, fn);
    break;
  case feature::GenerateInfo::OsmSourceType::O5M:
    ProcessOsmElementsFromO5M(reader, fn);
    break;
  }

  LOG(LINFO, ("Processing", info.m_osmFileName, "done."));
  MixFakeNodes(GetPlatform().ResourcesDir() + MIXED_NODES_FILE, fn);
  if (!translators.Finish())
    return false;

  translators.GetNames(info.m_bucketNames);
  return true;
}

LoaderWrapper::LoaderWrapper(feature::GenerateInfo & info)
  : m_reader(cache::CreatePointStorageReader(info.m_nodeStorageType, info.GetIntermediateFileName(NODES_FILE)), info)
{
  m_reader.LoadIndex();
}

cache::IntermediateDataReader & LoaderWrapper::GetReader()
{
  return m_reader;
}

CacheLoader::CacheLoader(feature::GenerateInfo & info) : m_info(info) {}

cache::IntermediateDataReader & CacheLoader::GetCache()
{
    if (!m_loader)
      m_loader = std::make_unique<LoaderWrapper>(m_info);

    return m_loader->GetReader();
}

bool GenerateIntermediateData(feature::GenerateInfo & info)
{
  try
  {
    auto nodes = cache::CreatePointStorageWriter(info.m_nodeStorageType,
                                                 info.GetIntermediateFileName(NODES_FILE));
    cache::IntermediateDataWriter cache(nodes, info);
    TownsDumper towns;
    CameraNodeIntermediateDataProcessor cameras(info.GetIntermediateFileName(CAMERAS_NODES_TO_WAYS_FILE),
                                                info.GetIntermediateFileName(CAMERAS_MAXSPEED_FILE));

    SourceReader reader = info.m_osmFileName.empty() ? SourceReader() : SourceReader(info.m_osmFileName);

    LOG(LINFO, ("Data source:", info.m_osmFileName));

    switch (info.m_osmFileType)
    {
    case feature::GenerateInfo::OsmSourceType::XML:
      BuildIntermediateDataFromXML(reader, cache, towns, cameras);
      break;
    case feature::GenerateInfo::OsmSourceType::O5M:
      BuildIntermediateDataFromO5M(reader, cache, towns, cameras);
      break;
    }

    cache.SaveIndex();
    cameras.SaveIndex();
    towns.Dump(info.GetIntermediateFileName(TOWNS_FILE));
    LOG(LINFO, ("Added points count =", nodes->GetNumProcessedPoints()));
  }
  catch (Writer::Exception const & e)
  {
    LOG(LCRITICAL, ("Error with file:", e.what()));
  }
  return true;
}

}  // namespace generator
