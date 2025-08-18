#include "testing/testing.hpp"

#include "drape_frontend/frame_values.hpp"

#include "drape/glsl_types.hpp"

#include "shaders/program_params.hpp"

namespace
{
glsl::mat4 const kTestMatrix1 =
    glsl::mat4(1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f);
glsl::mat4 const kTestMatrix2 =
    glsl::mat4(4.0f, 3.0f, 2.0f, 1.0f, 4.0f, 3.0f, 2.0f, 1.0f, 4.0f, 3.0f, 2.0f, 1.0f, 4.0f, 3.0f, 2.0f, 1.0f);
}  // namespace

UNIT_TEST(FrameValues_SetTo)
{
  df::FrameValues frameValues;
  frameValues.m_pivotTransform = kTestMatrix1;
  frameValues.m_projection = kTestMatrix2;
  frameValues.m_zScale = 42.0f;

  gpu::MapProgramParams params;
  frameValues.SetTo(params);

  TEST(params.m_pivotTransform == frameValues.m_pivotTransform, ());
  TEST(params.m_projection == frameValues.m_projection, ());
  TEST(params.m_zScale == frameValues.m_zScale, ());
}

UNIT_TEST(FrameValues_SetTo_Partial)
{
  df::FrameValues frameValues;
  frameValues.m_pivotTransform = kTestMatrix1;
  frameValues.m_projection = kTestMatrix2;
  gpu::RouteProgramParams params;
  frameValues.SetTo(params);

  TEST(params.m_pivotTransform == frameValues.m_pivotTransform, ());
  TEST(params.m_projection == frameValues.m_projection, ());
}
