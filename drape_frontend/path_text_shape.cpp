#include "path_text_shape.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_set_holder.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

namespace df
{

using m2::PointF;

namespace
{
  static float const realFontSize = 20.0f;
  float angelFromDir(float x, float y)
  {
    float gip = sqrtf(x*x + y*y);
    float cosa = x / gip;
    if(y > 0)
      return acosf(cosa) * 180.0f / M_PI;
    else
      return 360.0f - acosf(cosa) * 180.0f / M_PI;
  }
}

PathTextShape::PathTextShape(vector<PointF> const & path, TextViewParams const & params):
  m_params(params)
{
  m_path.FromArray(path);
}

void PathTextShape::Load(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  float scale = 1.0f;

  strings::UniString const text = strings::MakeUniString(m_params.m_primaryText);
  float const fontSize = m_params.m_primaryTextFont.m_size;

  // Fill buffers
  int cnt = text.size();
  float *uvs=new float[cnt*12];
  float *dirs=new float[cnt*12];
  float *colors=new float[cnt*24];
  for(int i=0;i<cnt*24;i++)
  {
    colors[i++]=0.0f;
    colors[i++]=0.0f;
    colors[i++]=0.0f;
    colors[i]=1.0f;
  }

  Spline::iterator itr;
  itr.Attach(m_path);
  itr.Step(0.0f);

  int textureSet;
  for (int i = 0 ; i < cnt ; i++)
  {
    TextureSetHolder::GlyphRegion region;
    textures->GetGlyphRegion(text[i], region);
    float xOffset, yOffset, advance;
    region.GetMetrics(xOffset, yOffset, advance);
    float const aspect = fontSize / realFontSize;
    advance *= aspect;

    textureSet = region.GetTextureNode().m_textureSet;
    m2::RectF const rect = region.GetTexRect();
    m2::PointU pixelSize;
    region.GetPixelSize(pixelSize);
//    float const h = pixelSize.y * aspect;
//    float const w = pixelSize.x * aspect;
    yOffset *= aspect;
    xOffset *= aspect;

    float uv_x = rect.minX();
    float uv_y = rect.minY();
    float h = rect.maxX() - rect.minX();
    float w = rect.maxY() - rect.minY();
    uvs[12*i+0]=uv_x;
    uvs[12*i+1]=uv_y;
    uvs[12*i+2]=uv_x;
    uvs[12*i+3]=(uv_y+h);
    uvs[12*i+4]=uv_x+w;
    uvs[12*i+5]=uv_y;

    uvs[12*i+6]=uv_x+w;
    uvs[12*i+7]=(uv_y+h);
    uvs[12*i+8]=uv_x+w;
    uvs[12*i+9]=uv_y;
    uvs[12*i+10]=uv_x;
    uvs[12*i+11]=(uv_y+h);

    PointF pos = itr.pos;
    PointF old_dir = itr.dir;

    int index1 = itr.index;
    itr.Step(advance / scale);
    int index2 = itr.index;
    PointF dir;
    if(index1 != index2)
    {
      PointF new_dir = itr.dir;
      PointF new_pos = m_path.position[index2];
      float smooth_factor = (new_pos - itr.pos).Length()/(advance / scale);
      dir = old_dir * (1.0f - smooth_factor) + new_dir * smooth_factor;
    }
    else
    {
      dir = old_dir;
    }

    float angle = angelFromDir(dir.x, dir.y) * M_PI / 180.0f;

    float cosa = cosf(angle);
    float sina = sinf(angle);
    float half_width = pixelSize.x * aspect / 2.0f;
    float half_height = pixelSize.y * aspect / 2.0f;

    float x1 = half_width * cosa - half_height * sina;
    float y1 = half_width * sina + half_height * cosa;

    float x2 = -half_width * cosa - half_height * sina;
    float y2 = -half_width * sina + half_height * cosa;

    float x3 = -x1;
    float y3 = -y1;

    float x4 = -x2;
    float y4 = -y2;

    x1 /= scale;
    x2 /= scale;
    x3 /= scale;
    x4 /= scale;

    y1 /= scale;
    y2 /= scale;
    y3 /= scale;
    y4 /= scale;

    x1 += pos.x;
    x2 += pos.x;
    x3 += pos.x;
    x4 += pos.x;

    y1 += pos.y;
    y2 += pos.y;
    y3 += pos.y;
    y4 += pos.y;

    dirs[12*i+0] = x2;
    dirs[12*i+1] = y2;
    dirs[12*i+2] = x3;
    dirs[12*i+3] = y3;
    dirs[12*i+4] = x1;
    dirs[12*i+5] = y1;

    dirs[12*i+6] = x4;
    dirs[12*i+7] = y4;
    dirs[12*i+8] = x1;
    dirs[12*i+9] = y1;
    dirs[12*i+10] = x3;
    dirs[12*i+11] = y3;
  }

  GLState state(gpu::PATH_FONT_PROGRAM, GLState::OverlayLayer);
  state.SetTextureSet(textureSet);
  state.SetBlending(Blending(true));

  AttributeProvider provider(3, 6*cnt);
  {
    BindingInfo position(1);
    BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, MakeStackRefPointer(dirs));
  }
  {
    BindingInfo texcoord(1);
    BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texcoord";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, texcoord, MakeStackRefPointer(uvs));
  }
  {
    BindingInfo base_color(1);
    BindingDecl & decl = base_color.GetBindingDecl(0);
    decl.m_attributeName = "a_color";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, base_color, MakeStackRefPointer(colors));
  }

  batcher->InsertTriangleList(state, MakeStackRefPointer(&provider));

  delete [] uvs;
  delete [] dirs;
  delete [] colors;
}

void PathTextShape::update(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{

}

void PathTextShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  Load(batcher, textures);
}

}
