#include "drape/render_context.hpp"

namespace dp
{
thread_local std::shared_ptr<RenderContext> RenderContext::s_current;
std::atomic<size_t> RenderContext::s_generation{0};
}  // namespace dp
