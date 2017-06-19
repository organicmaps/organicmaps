#include "drape_frontend/user_mark_builder.hpp"

namespace df
{
UserMarkBuilder::UserMarkBuilder(TFlushFn const & flushFn)
  : m_flushFn(flushFn)
{

}

void UserMarkBuilder::BuildUserMarks(UserMarksRenderCollection const & marks, MarkIndexesCollection && indexes,
                                     ref_ptr<dp::TextureManager> textures)
{

}
}  // namespace df
