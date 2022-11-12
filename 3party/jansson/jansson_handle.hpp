#pragma once

#include <algorithm>

namespace base
{
template <typename T> class JsonHandle
{
  void IncRef();
  void DecRef();

public:
  JsonHandle(T * pJson = 0) : m_pJson(pJson)
  {
    IncRef();
  }

  JsonHandle(JsonHandle const & json) : m_pJson(json.m_pJson)
  {
    IncRef();
  }

  ~JsonHandle()
  {
    DecRef();
  }

  JsonHandle & operator = (JsonHandle const & json)
  {
    JsonHandle tmp(json);
    tmp.swap(*this);
    return *this;
  }

  void swap(JsonHandle & json)
  {
    std::swap(m_pJson, json.m_pJson);
  }

  T * get() const
  {
    return m_pJson;
  }

  T * operator -> () const
  {
    return m_pJson;
  }

  operator bool () const
  {
    return m_pJson != 0;
  }

  /// Attach newly created object without incrementing ref count (it's already == 1).
  void AttachNew(T * pJson)
  {
    DecRef();

    m_pJson = pJson;
  }

private:
  T * m_pJson;
};
}  // namespace base
