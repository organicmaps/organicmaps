#include "generator/holes.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

namespace generator
{
// HolesAccumulator
HolesAccumulator::HolesAccumulator(cache::IntermediateDataReader & holder) :
  m_merger(holder)
{
}

FeatureBuilder1::Geometry & HolesAccumulator::GetHoles()
{
  ASSERT(m_holes.empty(), ("Can call only once"));
  m_merger.ForEachArea(false, [this](FeatureBuilder1::PointSeq const & v,
                       std::vector<uint64_t> const &)
  {
    m_holes.push_back(std::move(v));
  });
  return m_holes;
}

// HolesProcessor
HolesProcessor::HolesProcessor(uint64_t id, cache::IntermediateDataReader & holder) :
  m_id(id),
  m_holes(holder)
{
}

bool HolesProcessor::operator() (uint64_t /*id*/, RelationElement const & e)
{
  std::string const type = e.GetType();
  if (!(type == "multipolygon" || type == "boundary"))
    return false;

  std::string role;
  if (e.FindWay(m_id, role) && role == "outer")
  {
    e.ForEachWay(*this);
    // stop processing (??? assume that "outer way" exists in one relation only ???)
    return true;
  }
  return false;
}

void HolesProcessor::operator() (uint64_t id, std::string const & role)
{
  if (id != m_id && role == "inner")
    m_holes(id);
}


HolesRelation::HolesRelation(cache::IntermediateDataReader & holder) :
  m_holes(holder),
  m_outer(holder)
{
}

void HolesRelation::Build(OsmElement const * p)
{
  // Iterate ways to get 'outer' and 'inner' geometries.
  for (auto const & e : p->Members())
  {
    if (e.type != OsmElement::EntityType::Way)
      continue;

    if (e.role == "outer")
      m_outer.AddWay(e.ref);
    else if (e.role == "inner")
      m_holes(e.ref);
  }
}
}  // namespace generator
