#pragma once

#include "glbuffer.hpp"

class DataBuffer : public GLBuffer
{
public:
  DataBuffer(uint16_t elementSize, uint16_t capacity);
};

