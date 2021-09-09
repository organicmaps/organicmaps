#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#include <QtGui/QImage>
#pragma GCC diagnostic pop

QImage CreateImage(uint32_t w, uint32_t h, const uint8_t * mem);
