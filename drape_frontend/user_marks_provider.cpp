#include "user_marks_provider.hpp"

namespace
{
df::MarkID GetNextUserMarkId()
{
  static uint32_t nextMarkId = 0;
  return static_cast<df::MarkID>(++nextMarkId);
}
}  // namespace

namespace df
{
UserPointMark::UserPointMark()
  : m_id(GetNextUserMarkId())
{
}


UserLineMark::UserLineMark()
  : m_id(GetNextUserMarkId())
{
}

UserMarksProvider::UserMarksProvider()
  : m_pendingOnDelete(false)
{
}

bool UserMarksProvider::IsPendingOnDelete()
{
  return m_pendingOnDelete;
}

void UserMarksProvider::DeleteLater()
{
  ASSERT(m_pendingOnDelete == false, ());
  m_pendingOnDelete = true;
}
}  // namespace df
