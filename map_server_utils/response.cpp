#include "response.hpp"

namespace srv
{
  Response::Response(QByteArray buffer)
    : m_buffer(buffer)
  {
  }
}
