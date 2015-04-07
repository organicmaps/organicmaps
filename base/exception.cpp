#include "base/exception.hpp"

char const * RootException::what() const throw()
{
  size_t const count = m_Msg.size();

  string asciiMsg;
  asciiMsg.resize(count);

  for (size_t i = 0; i < count; ++i)
  {
    if (static_cast<unsigned char>(m_Msg[i]) < 128)
      asciiMsg[i] = char(m_Msg[i]);
    else
      asciiMsg[i] = '?';
  }

  static string msg;
  msg = string(m_What) + ", \"" + asciiMsg + "\"";
  return msg.c_str();
}
