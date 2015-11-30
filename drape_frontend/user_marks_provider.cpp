#include "user_marks_provider.hpp"

df::UserMarksProvider::UserMarksProvider()
  : m_pendingOnDelete(false)
  , m_counter(0)
{
}

void df::UserMarksProvider::BeginRead()
{
  Lock();
}

bool df::UserMarksProvider::IsDirty() const
{
  return m_isDirty;
}

void df::UserMarksProvider::EndRead()
{
  m_isDirty = false;
  Unlock();
}

void df::UserMarksProvider::IncrementCounter()
{
  ASSERT(m_pendingOnDelete == false, ());
  ++m_counter;
}

void df::UserMarksProvider::DecrementCounter()
{
  ASSERT(m_counter > 0, ());
  --m_counter;
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

bool df::UserMarksProvider::CanBeDeleted()
{
  return m_counter == 0;
}

void df::UserMarksProvider::BeginWrite()
{
  Lock();
}

void df::UserMarksProvider::SetDirty()
{
  m_isDirty = true;
}

void df::UserMarksProvider::EndWrite()
{
  Unlock();
}

void df::UserMarksProvider::Lock()
{
  m_mutex.Lock();
}

void df::UserMarksProvider::Unlock()
{
  m_mutex.Unlock();
}
