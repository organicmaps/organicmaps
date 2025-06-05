#include "testing/testing.hpp"

#include "drape/attribute_provider.hpp"

#include <gmock/gmock.h>

using namespace dp;

UNIT_TEST(InitStreamsTest)
{
  int const VERTEX_COUNT = 10;
  AttributeProvider provider(3, VERTEX_COUNT);
  float positions[2 * VERTEX_COUNT];
  float depth[VERTEX_COUNT];
  float normals[2 * VERTEX_COUNT];

  for (int i = 0; i < VERTEX_COUNT; ++i)
  {
    positions[2 * i] = (float)i;
    positions[(2 * i) + 1] = 0.0f;
    depth[i] = (float)i;
    normals[2 * i] = (float)i;
    normals[(2 * i) + 1] = 0.0;
  }

  {
    BindingInfo zeroStreamBinding(1);
    BindingDecl & decl = zeroStreamBinding.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, zeroStreamBinding, make_ref(positions));
  }

  {
    BindingInfo firstStreamBinding(1);
    BindingDecl & decl = firstStreamBinding.GetBindingDecl(0);
    decl.m_attributeName = "depth";
    decl.m_componentCount = 1;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, firstStreamBinding, make_ref(depth));
  }

  {
    BindingInfo secondStreamBinding(1);
    BindingDecl & decl = secondStreamBinding.GetBindingDecl(0);
    decl.m_attributeName = "normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, secondStreamBinding, make_ref(normals));
  }

  TEST_EQUAL(provider.IsDataExists(), true, ());
  TEST_EQUAL(provider.GetVertexCount(), 10, ());
  TEST_EQUAL(provider.GetRawPointer(0), (void *)positions, ());
  TEST_EQUAL(provider.GetRawPointer(1), (void *)depth, ());
  TEST_EQUAL(provider.GetRawPointer(2), (void *)normals, ());

  provider.Advance(1);

  TEST_EQUAL(provider.IsDataExists(), true, ());
  TEST_EQUAL(provider.GetVertexCount(), 9, ());
  TEST_EQUAL(provider.GetRawPointer(0), (void *)(&positions[2]), ());
  TEST_EQUAL(provider.GetRawPointer(1), (void *)(&depth[1]), ());
  TEST_EQUAL(provider.GetRawPointer(2), (void *)(&normals[2]), ());

  provider.Advance(9);
  TEST_EQUAL(provider.IsDataExists(), false, ());
  TEST_EQUAL(provider.GetVertexCount(), 0, ());
}

UNIT_TEST(InterleavedStreamTest)
{
  int const VERTEX_COUNT = 10;
  AttributeProvider provider(1, 10);
  float data[5 * VERTEX_COUNT];

  for (int i = 0; i < VERTEX_COUNT; ++i)
  {
    data[(5 * i)] = (float)i;
    data[(5 * i) + 1] = 0.0;
    data[(5 * i) + 2] = (float)i;
    data[(5 * i) + 3] = (float)i;
    data[(5 * i) + 4] = 0.0;
  }

  BindingInfo binding(3);
  {
    BindingDecl & decl = binding.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 5 * sizeof(float);
  }
  {
    BindingDecl & decl = binding.GetBindingDecl(1);
    decl.m_attributeName = "depth";
    decl.m_componentCount = 1;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 2 * sizeof(float);
    decl.m_stride = 5 * sizeof(float);
  }
  {
    BindingDecl & decl = binding.GetBindingDecl(2);
    decl.m_attributeName = "normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 3 * sizeof(float);
    decl.m_stride = 5 * sizeof(float);
  }

  provider.InitStream(0, binding, make_ref(data));

  TEST_EQUAL(provider.IsDataExists(), true, ());
  TEST_EQUAL(provider.GetVertexCount(), 10, ());
  TEST_EQUAL(provider.GetRawPointer(0), (void *)data, ());

  provider.Advance(1);

  TEST_EQUAL(provider.IsDataExists(), true, ());
  TEST_EQUAL(provider.GetVertexCount(), 9, ());
  TEST_EQUAL(provider.GetRawPointer(0), (void *)(&data[5]), ());

  provider.Advance(9);
  TEST_EQUAL(provider.IsDataExists(), false, ());
  TEST_EQUAL(provider.GetVertexCount(), 0, ());
}

UNIT_TEST(MixedStreamsTest)
{
  int const VERTEX_COUNT = 10;
  AttributeProvider provider(2, 10);
  float position[3 * VERTEX_COUNT];
  float normal[2 * VERTEX_COUNT];

  for (int i = 0; i < VERTEX_COUNT; ++i)
  {
    position[3 * i] = (float)i;        // x
    position[(3 * i) + 1] = 0.0;       // y
    position[(3 * i) + 2] = (float)i;  // z
    position[2 * i] = (float)i;        // Nx
    position[(2 * i) + 1] = 0.0;       // Ny
  }

  {
    BindingInfo binding(2);
    {
      BindingDecl & decl = binding.GetBindingDecl(0);
      decl.m_attributeName = "position";
      decl.m_componentCount = 2;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 3 * sizeof(float);
    }

    {
      BindingDecl & decl = binding.GetBindingDecl(1);
      decl.m_attributeName = "depth";
      decl.m_componentCount = 1;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 2 * sizeof(float);
      decl.m_stride = 3 * sizeof(float);
    }

    provider.InitStream(0, binding, make_ref(position));
  }

  {
    BindingInfo binding(1);
    BindingDecl & decl = binding.GetBindingDecl(0);
    decl.m_attributeName = "normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, binding, make_ref(normal));
  }

  TEST_EQUAL(provider.IsDataExists(), true, ());
  TEST_EQUAL(provider.GetVertexCount(), 10, ());
  TEST_EQUAL(provider.GetRawPointer(0), (void *)position, ());
  TEST_EQUAL(provider.GetRawPointer(1), (void *)normal, ());

  provider.Advance(1);

  TEST_EQUAL(provider.IsDataExists(), true, ());
  TEST_EQUAL(provider.GetVertexCount(), 9, ());
  TEST_EQUAL(provider.GetRawPointer(0), (void *)(&position[3]), ());
  TEST_EQUAL(provider.GetRawPointer(1), (void *)(&normal[2]), ());

  provider.Advance(9);
  TEST_EQUAL(provider.IsDataExists(), false, ());
  TEST_EQUAL(provider.GetVertexCount(), 0, ());
}
