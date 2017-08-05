#pragma once

#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/render_state.hpp"

#include "drape/render_bucket.hpp"
#include "drape/texture_manager.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace df
{

struct DrapeApiRenderProperty
{
  string m_id;
  m2::PointD m_center;
  vector<pair<dp::GLState, drape_ptr<dp::RenderBucket>>> m_buckets;
};

class DrapeApiBuilder
{
public:
  DrapeApiBuilder() = default;

  void BuildLines(DrapeApi::TLines const & lines, ref_ptr<dp::TextureManager> textures,
                  vector<drape_ptr<DrapeApiRenderProperty>> & properties);
};

} // namespace df
