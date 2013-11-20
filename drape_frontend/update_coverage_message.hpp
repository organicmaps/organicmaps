#pragma once

#include "message.hpp"
#include "../geometry/screenbase.hpp"

namespace df
{
  class UpdateCoverageMessage : public Message
  {
  public:
    UpdateCoverageMessage(ScreenBase const & screen);

    const ScreenBase & GetScreen() const;

  private:
    ScreenBase m_screen;
  };
}
