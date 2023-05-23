#pragma once

#include "coding/string_utf8_multilang.hpp"

namespace kml
{
auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;
auto const kDefaultTrackWidth = 5.0;
auto const kDefaultTrackColor = 0x006ec7ff;

template <typename Channel>
uint32_t ToRGBA(Channel red, Channel green, Channel blue, Channel alpha)
{
  return static_cast<uint8_t>(red) << 24 | static_cast<uint8_t>(green) << 16 |
         static_cast<uint8_t>(blue) << 8 | static_cast<uint8_t>(alpha);
}

}
