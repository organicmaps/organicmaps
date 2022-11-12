#include "3party/jansson/jansson_handle.hpp"

#include <jansson.h>

namespace base
{
template <> void JsonHandle<json_t>::IncRef()
{
  if (m_pJson)
    json_incref(m_pJson);
}

template <> void JsonHandle<json_t>::DecRef()
{
  if (m_pJson)
    json_decref(m_pJson);
}
}  // namespace base
