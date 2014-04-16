#include "message.hpp"

#include "../base/assert.hpp"

namespace df
{

Message::Message()
  : m_type(Unknown) {}

Message::Type Message::GetType() const
{
  ASSERT(m_type != Unknown, ());
  return m_type;
}

void Message::SetType(Type t)
{
  m_type = t;
}

} // namespace df
