#include "texture_set_holder.hpp"

#include "font_texture.hpp"

namespace dp
{

TextureSetHolder::TextureNode::TextureNode()
  : m_width(-1)
  , m_height(-1)
  , m_textureSet(-1)
  , m_textureOffset(-1)
{
}

bool TextureSetHolder::TextureNode::IsValid() const
{
  return m_width != -1      &&
         m_height != -1     &&
         m_textureSet != -1 &&
         m_textureOffset != -1;
}


TextureSetHolder::BaseRegion::BaseRegion()
  : m_info(NULL)
{
}

bool TextureSetHolder::BaseRegion::IsValid() const
{
  return m_info != NULL && m_node.IsValid();
}

void TextureSetHolder::BaseRegion::SetResourceInfo(Texture::ResourceInfo const * info)
{
  m_info = info;
}

void TextureSetHolder::BaseRegion::SetTextureNode(TextureNode const & node)
{
  m_node = node;
}

void TextureSetHolder::BaseRegion::GetPixelSize(m2::PointU & size) const
{
  m2::RectF const & texRect = m_info->GetTexRect();
  size.x = texRect.SizeX() * m_node.m_width;
  size.y = texRect.SizeY() * m_node.m_height;
}

m2::RectF const & TextureSetHolder::BaseRegion::GetTexRect() const
{
  return m_info->GetTexRect();
}

TextureSetHolder::TextureNode const & TextureSetHolder::BaseRegion::GetTextureNode() const
{
  return m_node;
}

TextureSetHolder::GlyphRegion::GlyphRegion()
  : BaseRegion()
{
}

void TextureSetHolder::GlyphRegion::GetMetrics(float & xOffset, float & yOffset, float & advance) const
{
  ASSERT(m_info->GetType() == Texture::Glyph, ());
  FontTexture::GlyphInfo const * info = static_cast<FontTexture::GlyphInfo const *>(m_info);
  info->GetMetrics(xOffset, yOffset, advance);
}

float TextureSetHolder::GlyphRegion::GetAdvance() const
{
  ASSERT(m_info->GetType() == Texture::Glyph, ());
  return static_cast<FontTexture::GlyphInfo const *>(m_info)->GetAdvance();
}

uint32_t TextureSetHolder::StippleRegion::GetTemplateLength() const
{
  ASSERT(m_info->GetType() == Texture::StipplePen, ());
  return static_cast<StipplePenResourceInfo const *>(m_info)->GetPixelLength();
}

uint32_t TextureSetHolder::StippleRegion::GetPatternLength() const
{
  ASSERT(m_info->GetType() == Texture::StipplePen, ());
  return static_cast<StipplePenResourceInfo const *>(m_info)->GetPatternLength();
}

} // namespace dp
