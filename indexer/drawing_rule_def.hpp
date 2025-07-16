#pragma once

#include "base/buffer_vector.hpp"

#include "indexer/feature.hpp"

namespace drule
{

// Priority range for area and line drules. Should be same as LAYER_PRIORITY_RANGE in kothic.
// Each layer = +/-1 value shifts the range by this number, so that e.g. priorities
// of the default layer=0 range [0;1000) don't intersect with layer=-1 range [-1000;0) and so on..
double constexpr kLayerPriorityRange = 1000;

// Should be same as OVERLAYS_MAX_PRIORITY in kothic.
// The overlays range is [-kOverlaysMaxPriority; kOverlaysMaxPriority), negative values are used
// for optional captions which are below all other overlays.
int32_t constexpr kOverlaysMaxPriority = 10000;

/*
 * Besides overlays, the overall rendering depth range [dp::kMinDepth;dp::kMaxDepth] is divided
 * into following specific depth ranges:
 * FG - foreground lines and areas (buildings..), rendered on top of other geometry always,
 *      even if a fg feature is layer=-10 (e.g. tunnels should be visibile over landcover and water).
 * BG-top - rendered just on top of BG-by-size range, because ordering by size
 *      doesn't always work with e.g. water mapped over a forest,
 *      so water should be on top of other landcover always,
 *      but linear waterways should be hidden beneath it.
 * BG-by-size - landcover areas rendered in bbox size order, smaller areas are above larger ones.
 * Still, a BG-top water area with layer=-1 should go below other landcover,
 * and a layer=1 landcover area should be displayed above water,
 * so BG-top and BG-by-size should share the same "layer" space.
 */

// Priority values coming from style files are expected to be separated into following ranges.
static double constexpr kBasePriorityFg = 0, kBasePriorityBgTop = -1000, kBasePriorityBgBySize = -2000;

// Define depth ranges boundaries to accomodate for all possible layer=* values.
// One layer space is drule::kLayerPriorityRange (1000). So each +1/-1 layer value shifts
// depth of the drule by -1000/+1000.

// FG depth range: [0;1000).
static double constexpr kBaseDepthFg = 0,
                        // layer=-10/10 gives an overall FG range of [-10000;11000).
    kMaxLayeredDepthFg = kBaseDepthFg + (1 + feature::LAYER_HIGH) * kLayerPriorityRange,
                        kMinLayeredDepthFg = kBaseDepthFg + int8_t{feature::LAYER_LOW} * kLayerPriorityRange,
                        // Split the background layer space as 100 for BG-top and 900 for BG-by-size.
    kBgTopRangeFraction = 0.1, kDepthRangeBgTop = kBgTopRangeFraction * kLayerPriorityRange,
                        kDepthRangeBgBySize = kLayerPriorityRange - kDepthRangeBgTop,
                        // So the BG-top range is [-10100,-10000).
    kBaseDepthBgTop = kMinLayeredDepthFg - kDepthRangeBgTop,
                        // And BG-by-size range is [-11000,-10100).
    kBaseDepthBgBySize = kBaseDepthBgTop - kDepthRangeBgBySize,
                        // Minimum BG depth for layer=-10 is -21000.
    kMinLayeredDepthBg = kBaseDepthBgBySize + int8_t{feature::LAYER_LOW} * kLayerPriorityRange;

class Key
{
public:
  uint8_t m_scale = -1;
  int m_type = -1;
  size_t m_index = std::numeric_limits<size_t>::max();  // an index to RulesHolder.m_dRules[]
  int m_priority = -1;
  bool m_hatching = false;

  Key() = default;
  Key(uint8_t s, int t, size_t i) : m_scale(s), m_type(t), m_index(i), m_priority(-1) {}

  bool operator==(Key const & r) const { return (m_index == r.m_index); }

  void SetPriority(int pr) { m_priority = pr; }
};

/// drawing type of rule - can be one of ...
enum TypeT
{
  line,
  area,
  symbol,
  caption,
  circle,
  pathtext,
  waymarker,
  shield,
  count_of_rules
};

typedef buffer_vector<Key, 16> KeysT;
void MakeUnique(KeysT & keys);
double CalcAreaBySizeDepth(FeatureType & f);
}  // namespace drule
