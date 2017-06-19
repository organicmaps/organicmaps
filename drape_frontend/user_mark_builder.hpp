#pragma once

#include "drape_frontend/user_mark_shapes.hpp"
#include "drape_frontend/user_mark_generator.hpp"

namespace df
{
class UserMarkBuilder
{
public:
  using TFlushFn = function<void(TUserMarkShapes && shapes)>;

  UserMarkBuilder(TFlushFn const & flushFn);

  void BuildUserMarks(MarkGroups const & marks, MarkIndexesGroups && indexes,
                      ref_ptr<dp::TextureManager> textures);
private:
  TFlushFn m_flushFn;
};
}
