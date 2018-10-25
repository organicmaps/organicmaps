#pragma once

#include "metrics/eye_info.hpp"

#include "base/exception.hpp"

#include <cstdint>
#include <vector>

namespace eye
{
class Serdes
{
public:
  DECLARE_EXCEPTION(UnknownVersion, RootException);

  static void SerializeInfo(Info const & info, std::vector<int8_t> & result);
  static void DeserializeInfo(std::vector<int8_t> const & bytes, Info & result);
  static void SerializeMapObjects(MapObjects const & mapObjects, std::vector<int8_t> & result);
  static void DeserializeMapObjects(std::vector<int8_t> const & bytes, MapObjects & result);
  static void SerializeMapObjectEvent(MapObject const & poi, MapObject::Event const & poiEvent,
                                      std::vector<int8_t> & result);
};
}  // namespace eye
