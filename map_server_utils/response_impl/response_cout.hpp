#pragma once

#include "../response.hpp"

namespace srv
{
  namespace impl
  {
    class ResponseCout : public ::srv::Response
    {
      typedef ::srv::Response base_t;
    public:
      ResponseCout(QByteArray buffer);

      virtual void DoResponse() const;
    };
  }
}
