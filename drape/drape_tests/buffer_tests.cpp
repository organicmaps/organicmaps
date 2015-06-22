#include "testing/testing.hpp"

#include "drape/drape_tests/glmock_functions.hpp"

#include "drape/data_buffer.hpp"
#include "drape/gpu_buffer.hpp"
#include "drape/index_buffer.hpp"
#include "drape/index_storage.hpp"

#include "std/cstdlib.hpp"
#include "std/unique_ptr.hpp"

#include <gmock/gmock.h>

using namespace emul;
using namespace dp;

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

UNIT_TEST(CreateDestroyDataBufferTest)
{
  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, 3 * 100 * sizeof(float), NULL, gl_const::GLDynamicDraw));
  EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
  EXPECTGL(glDeleteBuffer(1));

  unique_ptr<DataBuffer> buffer(new DataBuffer(3 * sizeof(float), 100));
  buffer->MoveToGPU(GPUBuffer::ElementBuffer);
}

UNIT_TEST(CreateDestroyIndexBufferTest)
{
  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLElementArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, 100 * dp::IndexStorage::SizeOfIndex(), NULL, gl_const::GLDynamicDraw));
  EXPECTGL(glBindBuffer(0, gl_const::GLElementArrayBuffer));
  EXPECTGL(glDeleteBuffer(1));

  unique_ptr<DataBuffer> buffer(new IndexBuffer(100));
  buffer->MoveToGPU(GPUBuffer::IndexBuffer);
}

UNIT_TEST(UploadDataTest)
{
  float data[3 * 100];
  for (int i = 0; i < 3 * 100; ++i)
    data[i] = (float)i;

  unique_ptr<DataBuffer> buffer(new DataBuffer(3 * sizeof(float), 100));

  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, 3 * 100 * sizeof(float), buffer->GetBuffer()->Data(), gl_const::GLDynamicDraw));
  EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
  EXPECTGL(glDeleteBuffer(1));

  buffer->GetBuffer()->UploadData(data, 100);
  buffer->MoveToGPU(GPUBuffer::ElementBuffer);
}

UNIT_TEST(ParticalUploadDataTest)
{
  size_t const kPart1Size = 3 * 30;
  float part1Data[kPart1Size];
  for (int i = 0; i < kPart1Size; ++i)
    part1Data[i] = (float)i;

  size_t const kPart2Size = 3 * 100;
  float part2Data[kPart2Size];
  for (int i = 0; i < kPart2Size; ++i)
    part2Data[i] = (float)i;

  unique_ptr<DataBuffer> buffer(new DataBuffer(3 * sizeof(float), 100));

  InSequence s;
  EXPECTGL(glGenBuffer()).WillOnce(Return(1));
  EXPECTGL(glBindBuffer(1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, 3 * 100 * sizeof(float), buffer->GetBuffer()->Data(), gl_const::GLDynamicDraw));
  EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
  EXPECTGL(glDeleteBuffer(1));

  TEST_EQUAL(buffer->GetBuffer()->GetCapacity(), 100, ());
  TEST_EQUAL(buffer->GetBuffer()->GetAvailableSize(), 100, ());
  TEST_EQUAL(buffer->GetBuffer()->GetCurrentSize(), 0, ());

  buffer->GetBuffer()->UploadData(part1Data, 30);
  TEST_EQUAL(buffer->GetBuffer()->GetCapacity(), 100, ());
  TEST_EQUAL(buffer->GetBuffer()->GetAvailableSize(), 70, ());
  TEST_EQUAL(buffer->GetBuffer()->GetCurrentSize(), 30, ());

  buffer->GetBuffer()->UploadData(part2Data, 70);
  TEST_EQUAL(buffer->GetBuffer()->GetCapacity(), 100, ());
  TEST_EQUAL(buffer->GetBuffer()->GetAvailableSize(), 0, ());
  TEST_EQUAL(buffer->GetBuffer()->GetCurrentSize(), 100, ());

  buffer->MoveToGPU(GPUBuffer::ElementBuffer);
}
