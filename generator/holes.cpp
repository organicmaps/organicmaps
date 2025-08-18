#include "generator/holes.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include <utility>

using namespace feature;

namespace generator
{
HolesAccumulator::HolesAccumulator(std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache)
  : m_merger(cache)
{}

FeatureBuilder::Geometry & HolesAccumulator::GetHoles()
{
  ASSERT(m_holes.empty(), ("It is allowed to call only once."));
  m_merger.ForEachArea(false,
                       [this](FeatureBuilder::PointSeq const & v, std::vector<uint64_t> const & /* way osm ids */)
  { m_holes.push_back(std::move(v)); });
  return m_holes;
}

HolesProcessor::HolesProcessor(uint64_t id, std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache)
  : m_id(id)
  , m_holes(cache)
{}

base::ControlFlow HolesProcessor::operator()(uint64_t /* id */, RelationElement const & e)
{
  auto const type = e.GetType();
  if (!(type == "multipolygon" || type == "boundary"))
    return base::ControlFlow::Continue;

  if (auto const role = e.GetWayRole(m_id); role == "outer")
  {
    e.ForEachWay(*this);
    // Stop processing. Assume that "outer way" exists in one relation only.
    return base::ControlFlow::Break;
  }
  return base::ControlFlow::Continue;
}

void HolesProcessor::operator()(uint64_t id, std::string const & role)
{
  if (id != m_id && role == "inner")
    m_holes(id);
}

HolesRelation::HolesRelation(std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache)
  : m_holes(cache)
  , m_outer(cache)
{}

void HolesRelation::Build(OsmElement const * p)
{
  // Iterate ways to get 'outer' and 'inner' geometries.
  for (auto const & e : p->Members())
  {
    if (e.m_type != OsmElement::EntityType::Way)
      continue;

    if (e.m_role == "outer")
      m_outer.AddWay(e.m_ref);
    else if (e.m_role == "inner")
      m_holes(e.m_ref);
  }
}
}  // namespace generator
