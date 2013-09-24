#pragma once

#include "glbuffer.hpp"

class DataBuffer : public GLBuffer
{
public:
  DataBuffer(uint8_t elementSize, uint16_t capacity);
};

