#include "response_cout.hpp"

#include "../std/string.hpp"
#include "../std/iostream.hpp"

namespace srv
{
  namespace impl
  {
    ResponseCout::ResponseCout(QByteArray buffer)
      : base_t(buffer)
    {
    }

    void ResponseCout::DoResponse() const
    {
      string data(m_buffer.constData(), m_buffer.size());
      cout << data;
    }
  }
}
