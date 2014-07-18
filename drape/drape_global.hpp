#pragma once

namespace dp
{

enum Anchor
{
  Center      = 0,
  Left        = 0x1,
  Right       = Left << 1,
  Top         = Right << 1,
  Bottom      = Top << 1,
  LeftTop     = Left | Top,
  RightTop    = Right | Top,
  LeftBottom  = Left | Bottom,
  RightBottom = Right | Bottom
};

enum LineCap
{
  SquareCap = -1,
  RoundCap  = 0,
  ButtCap   = 1,
};

enum LineJoin
{
  MiterJoin   = -1,
  BevelJoin  = 0,
  RoundJoin = 1,
};

}
