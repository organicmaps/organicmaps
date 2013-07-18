#pragma once

#include <QtCore/QByteArray>

namespace srv
{
  class Response
  {
  public:
    Response(QByteArray buffer);
    virtual ~Response() {};

    virtual void DoResponse() const = 0;

  protected:
    QByteArray m_buffer;
  };
}
