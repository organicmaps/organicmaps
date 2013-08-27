#include "jansson_handle.hpp"

#include <jansson.h>


namespace my
{

void JsonHandle::IncRef()
{
  if (m_pJson)
    json_incref(m_pJson);
}

void JsonHandle::DecRef()
{
  if (m_pJson)
    json_decref(m_pJson);
}

}
