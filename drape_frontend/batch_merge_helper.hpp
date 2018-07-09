#pragma once

#include "drape/pointers.hpp"
#include "drape_frontend/render_group.hpp"

#include <vector>

namespace dp
{
class VertexArrayBuffer;
}  // namespace dp

namespace gpu
{
class ProgramManager;
}  // namespace gpu

namespace df
{
class BatchMergeHelper
{
public:
  static bool IsMergeSupported();

  static void MergeBatches(ref_ptr<gpu::ProgramManager> mng, bool isPerspective,
                           std::vector<drape_ptr<RenderGroup>> & batches,
                           std::vector<drape_ptr<RenderGroup>> & mergedBatches);
};
}  // namespace df
