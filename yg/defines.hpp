#pragma once

namespace yg
{
  static const int maxDepth = 20000;

  enum EPosition
  {
    EPosCenter = 0x00,
    EPosAbove = 0x01,
    EPosUnder = 0x02,
    EPosLeft = 0x04,
    EPosRight = 0x10,
    EPosAboveLeft = EPosAbove | EPosLeft,
    EPosAboveRight = EPosAbove | EPosRight,
    EPosUnderLeft = EPosUnder | EPosLeft,
    EPosUnderRight = EPosUnder | EPosRight
  };
}
