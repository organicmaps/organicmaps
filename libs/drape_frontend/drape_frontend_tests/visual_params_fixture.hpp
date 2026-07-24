#pragma once

#include "drape_frontend/visual_params.hpp"

namespace df::test_support
{
/// Initializes the process-wide VisualParams singleton, which drape_frontend code reads and asserts
/// on if it was not initialized. Derive tests from it with UNIT_CLASS_TEST instead of calling
/// VisualParams::Init in the test body, so that they don't depend on another test in the binary
/// having initialized it first.
class VisualParamsFixture
{
public:
  VisualParamsFixture() { VisualParams::Init(1.0 /* visualScale */, 1024 /* tileSize */); }
};
}  // namespace df::test_support
