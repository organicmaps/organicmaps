#include "testing/testing.hpp"

#include "drape_frontend/frame_values.hpp"

#include "drape/glsl_types.hpp"

#include "shaders/program_params.hpp"

UNIT_TEST(FrameValues_SetTo)
{
  df::FrameValues frameValues;
  frameValues.m_pivotTransform = glsl::mat4(1.0, 2.0f, 3.0f, 4.0, 1.0, 2.0f, 3.0f, 4.0, 1.0, 2.0f,
                                            3.0f, 4.0, 1.0, 2.0f, 3.0f, 4.0);
  frameValues.m_projection = glsl::mat4(4.0, 3.0f, 2.0f, 1.0, 4.0, 3.0f, 2.0f, 1.0, 4.0, 3.0f, 2.0f,
                                        1.0, 4.0, 3.0f, 2.0f, 1.0);
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
  frameValues.m_pivotTransform = glsl::mat4(1.0, 2.0f, 3.0f, 4.0, 1.0, 2.0f, 3.0f, 4.0, 1.0, 2.0f,
                                            3.0f, 4.0, 1.0, 2.0f, 3.0f, 4.0);
  frameValues.m_projection = glsl::mat4(4.0, 3.0f, 2.0f, 1.0, 4.0, 3.0f, 2.0f, 1.0, 4.0, 3.0f, 2.0f,
                                        1.0, 4.0, 3.0f, 2.0f, 1.0);
  gpu::RouteProgramParams params;
  frameValues.SetTo(params);

  TEST(params.m_pivotTransform == frameValues.m_pivotTransform, ());
  TEST(params.m_projection == frameValues.m_projection, ());
}
