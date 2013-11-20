#pragma once

#include "message.hpp"

namespace df
{
  class DropCoverageMessage : public Message
  {
  public:
    DropCoverageMessage()
    {
      SetType(DropCoverage);
    }
  };
}
