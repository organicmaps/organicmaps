#include "base/exception.hpp"

RootException::RootException(char const * what, std::string const & msg) : m_msg(msg)
{
  std::string asciiMsg(m_msg.size(), '?');

  for (size_t i = 0; i < m_msg.size(); ++i)
    if (static_cast<unsigned char>(m_msg[i]) < 128)
      asciiMsg[i] = static_cast<char>(m_msg[i]);

  m_whatWithAscii = std::string(what) + ", \"" + asciiMsg + "\"";
}
