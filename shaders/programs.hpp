#pragma once

#include "base/assert.hpp"

#include <string>

namespace gpu
{
// Programs in enum are in the order of rendering priority.
// Each program should have shaders assigned in GL/shader_index.txt and metal_program_pool.mm
enum class Program
{
  ColoredSymbol = 0,
  Texturing,
  MaskedTexturing,
  Bookmark,
  BookmarkAnim,
  TextOutlined,
  Text,
  TextStaticOutlinedGui,
  TextOutlinedGui,
  Area,
  AreaOutline,
  Area3d,
  Area3dOutline,
  Line,
  TransitCircle,
  DashedLine,
  PathSymbol,
  TransparentArea,
  CapJoin,
  HatchingArea,
  TexturingGui,
  Ruler,
  Accuracy,
  MyPosition,
  SelectionLine,
  Transit,
  TransitMarker,
  Route,
  RouteDash,
  RouteArrow,
  RouteMarker,
  CirclePoint,
  BookmarkAboveText,
  BookmarkAnimAboveText,
  DebugRect,
  ScreenQuad,
  Arrow3d,
  Arrow3dTextured,
  Arrow3dShadow,
  Arrow3dOutline,
  ColoredSymbolBillboard,
  TexturingBillboard,
  MaskedTexturingBillboard,
  BookmarkBillboard,
  BookmarkAnimBillboard,
  BookmarkAboveTextBillboard,
  BookmarkAnimAboveTextBillboard,
  TextOutlinedBillboard,
  TextBillboard,
  Traffic,
  TrafficLine,
  TrafficCircle,
  SmaaEdges,
  SmaaBlendingWeight,
  SmaaFinal,

  ProgramsCount
};
}  // namespace gpu
