#pragma once

#include "glbuffer.hpp"

class IndexBuffer : public GLBuffer
{
public:
  IndexBuffer(uint16_t capacity);

  void UploadData(uint16_t * data, uint16_t size);
};
