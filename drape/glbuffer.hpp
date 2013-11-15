#pragma once

#include "../std/stdint.hpp"

class GLBuffer
{
public:
  enum Target
  {
    ElementBuffer,
    IndexBuffer
  };

public:
  GLBuffer(Target t, uint8_t elementSize, uint16_t capacity);
  ~GLBuffer();

  void UploadData(const void * data, uint16_t elementCount);
  uint16_t GetCapacity() const;
  uint16_t GetCurrentSize() const;
  uint16_t GetAvailableSize() const;

  void Bind();

private:
  Target m_t;
  uint8_t m_elementSize;
  uint16_t m_capacity;
  uint16_t m_size;

private:
  uint32_t m_bufferID;
};
