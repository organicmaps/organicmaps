#include "generator/composite_id.hpp"

#include <sstream>
#include <tuple>

namespace generator
{
CompositeId::CompositeId(std::string const & str)
{
  std::stringstream stream(str);
  stream.exceptions(std::ios::failbit);
  stream >> m_mainId;
  stream >> m_additionalId;
}

CompositeId::CompositeId(base::GeoObjectId mainId, base::GeoObjectId additionalId)
  : m_mainId(mainId)
  , m_additionalId(additionalId)
{}

CompositeId::CompositeId(base::GeoObjectId mainId) : CompositeId(mainId, base::GeoObjectId() /* additionalId */) {}

bool CompositeId::operator<(CompositeId const & other) const
{
  return std::tie(m_mainId, m_additionalId) < std::tie(other.m_mainId, other.m_additionalId);
}

bool CompositeId::operator==(CompositeId const & other) const
{
  return std::tie(m_mainId, m_additionalId) == std::tie(other.m_mainId, other.m_additionalId);
}

bool CompositeId::operator!=(CompositeId const & other) const
{
  return !(*this == other);
}

std::string CompositeId::ToString() const
{
  std::stringstream stream;
  stream.exceptions(std::ios::failbit);
  stream << m_mainId << " " << m_additionalId;
  return stream.str();
}

std::string DebugPrint(CompositeId const & id)
{
  return DebugPrint(id.m_mainId) + "|" + DebugPrint(id.m_additionalId);
}
}  // namespace generator
