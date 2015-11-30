#pragma once

#include "std/algorithm.hpp"


struct json_struct_t;

namespace my
{

class JsonHandle
{
  void IncRef();
  void DecRef();

public:
  JsonHandle(json_struct_t * pJson = 0) : m_pJson(pJson)
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
    ::swap(m_pJson, json.m_pJson);
  }

  json_struct_t * get() const
  {
    return m_pJson;
  }

  json_struct_t * operator -> () const
  {
    return m_pJson;
  }

  operator bool () const
  {
    return m_pJson != 0;
  }

  /// Attach newly created object without incrementing ref count (it's already == 1).
  void AttachNew(json_struct_t * pJson)
  {
    DecRef();

    m_pJson = pJson;
  }

private:
  json_struct_t * m_pJson;
};

}
