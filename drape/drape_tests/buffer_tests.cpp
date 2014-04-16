#include "../../testing/testing.hpp"

#include "glmock_functions.hpp"

#include "../gpu_buffer.hpp"
#include "../data_buffer.hpp"
#include "../index_buffer.hpp"

#include <gmock/gmock.h>

using namespace emul;
using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

UNIT_TEST(CreateDestroyDataBufferTest)
{
  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, 3 * 100 * sizeof(float), NULL, gl_const::GLStaticDraw));
  EXPECTGL(glDeleteBuffer(1));

  GPUBuffer * buffer = new DataBuffer(3 * sizeof(float), 100);
  delete buffer;
}

UNIT_TEST(CreateDestroyIndexBufferTest)
{
  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLElementArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, 100 * sizeof(uint16_t), NULL, gl_const::GLStaticDraw));
  EXPECTGL(glDeleteBuffer(1));

  GPUBuffer * buffer = new IndexBuffer(100);
  delete buffer;
}

UNIT_TEST(UploadDataTest)
{
  float data[3 * 100];
  for (int i = 0; i < 3 * 100; ++i)
    data[i] = (float)i;

  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, 3 * 100 * sizeof(float), NULL, gl_const::GLStaticDraw));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferSubData(gl_const::GLArrayBuffer, 3 * 100 * sizeof(float), data, 0));
  EXPECTGL(glDeleteBuffer(1));

  GPUBuffer * buffer = new GPUBuffer(GPUBuffer::ElementBuffer, 3 * sizeof(float), 100);
  buffer->UploadData(data, 100);
  delete buffer;
}

UNIT_TEST(ParticalUploadDataTest)
{
  float part1Data[3 * 30];
  for (int i = 0; i < 3 * 30; ++i)
    part1Data[i] = (float)i;

  float part2Data[3 * 70];
  for (int i = 0; i < 3 * 100; ++i)
    part2Data[i] = (float)i;

  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, 3 * 100 * sizeof(float), NULL, gl_const::GLStaticDraw));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferSubData(gl_const::GLArrayBuffer, 3 * 30 * sizeof(float), part1Data, 0));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferSubData(gl_const::GLArrayBuffer, 3 * 70 * sizeof(float), part2Data, 3 * 30 * sizeof(float)));
  EXPECTGL(glDeleteBuffer(1));

  GPUBuffer * buffer = new GPUBuffer(GPUBuffer::ElementBuffer, 3 * sizeof(float), 100);
  TEST_EQUAL(buffer->GetCapacity(), 100, ());
  TEST_EQUAL(buffer->GetAvailableSize(), 100, ());
  TEST_EQUAL(buffer->GetCurrentSize(), 0, ());

  buffer->UploadData(part1Data, 30);
  TEST_EQUAL(buffer->GetCapacity(), 100, ());
  TEST_EQUAL(buffer->GetAvailableSize(), 70, ());
  TEST_EQUAL(buffer->GetCurrentSize(), 30, ());

  buffer->UploadData(part2Data , 70);
  TEST_EQUAL(buffer->GetCapacity(), 100, ());
  TEST_EQUAL(buffer->GetAvailableSize(), 0, ());
  TEST_EQUAL(buffer->GetCurrentSize(), 100, ());

  delete buffer;
}
