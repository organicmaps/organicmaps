#include "route_relation.hpp"

namespace feature
{
RelationReader::Type RelationReader::GetType()
{
  if (!m_typeR)
  {
    m_rel.m_type = static_cast<Type>(ReadPrimitiveFromSource<uint8_t>(m_src));
    m_typeR = true;
  }
  return m_rel.m_type;
}

dp::Color RelationReader::GetColor()
{
  if (!m_colorR)
  {
    (void)GetType();

    m_flags = ReadPrimitiveFromSource<uint8_t>(m_src);
    if (m_flags & RouteRelationBase::HasColor)
      m_rel.m_color = dp::Color::FromRGBA(ReadPrimitiveFromSource<uint32_t>(m_src));

    m_colorR = true;
  }
  return m_rel.m_color;
}

RouteRelationBase const & RelationReader::GetRel()
{
  if (!m_otherR)
  {
    (void)GetColor();

    m_rel.ReadOther(m_src, m_flags);
    m_otherR = true;
  }
  return m_rel;
}

uint32_t RouteRelation::GetMember(int idx) const
{
  int const sz = static_cast<int>(m_ftMembers.size());
  if (idx >= 0)
  {
    ASSERT_LESS(idx, sz, ());
    return m_ftMembers[idx];
  }
  else
  {
    idx += sz;
    ASSERT_GREATER(idx, -1, ());
    return m_ftMembers[idx];
  }
}

}  // namespace feature
