#include "drape/drape_tests/img.hpp"

void cleanUpQImageMemory(void * mem)
{
  free(mem);
}

QImage CreateImage(uint32_t w, uint32_t h, uint8_t const * mem)
{
  int pitch = 32 * (((w - 1) / 32) + 1);
  int byteCount = pitch * h;
  unsigned char * buf = (unsigned char *)malloc(byteCount);
  memset(buf, 0, byteCount);
  for (uint32_t i = 0; i < h; ++i)
    memcpy(buf + pitch * i, mem + w * i, w);

  QImage img = QImage(buf, pitch, h, QImage::Format_Indexed8, &cleanUpQImageMemory, buf);

  img.setColorCount(0xFF);
  for (int i = 0; i < 256; ++i)
    img.setColor(i, qRgb(255 - i, 255 - i, 255 - i));

  return img;
}
