#include "update_coverage_message.hpp"

namespace df
{
  UpdateCoverageMessage::UpdateCoverageMessage(ScreenBase const & screen)
    : m_screen(screen)
  {
    SetType(UpdateCoverage);
  }

  const ScreenBase & UpdateCoverageMessage::GetScreen() const
  {
    return m_screen;
  }
}
