#include "user_marks_provider.hpp"

df::UserMarksProvider::UserMarksProvider()
  : m_pendingOnDelete(false)
{
}

bool df::UserMarksProvider::IsDirty() const
{
  return m_isDirty;
}

void df::UserMarksProvider::ResetDirty()
{
  m_isDirty = false;
}

bool df::UserMarksProvider::IsPendingOnDelete()
{
  return m_pendingOnDelete;
}

void df::UserMarksProvider::DeleteLater()
{
  ASSERT(m_pendingOnDelete == false, ());
  m_pendingOnDelete = true;
}

void df::UserMarksProvider::SetDirty()
{
  m_isDirty = true;
}
