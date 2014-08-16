#include "texture_manager.hpp"
#include "symbols_texture.hpp"
#include "font_texture.hpp"
#include "dynamic_texture.hpp"
#include "stipple_pen_resource.hpp"

#include "glfunctions.hpp"

#include "../platform/platform.hpp"

#include "../coding/file_name_utils.hpp"

#include "../base/stl_add.hpp"

#include "../std/vector.hpp"
#include "../std/bind.hpp"

namespace dp
{

class TextureManager::TextureSet
{
public:
  TextureSet(uint32_t maxSize)
    : m_maxSize(maxSize)
  {
  }

  ~TextureSet()
  {
    GetRangeDeletor(m_textures, MasterPointerDeleter())();
  }

  bool IsFull() const
  {
    return !(m_textures.size() < m_maxSize);
  }

  void AddTexture(TransferPointer<Texture> texture)
  {
    ASSERT(!IsFull(), ());
    m_textures.push_back(MasterPointer<Texture>(texture));
  }

  Texture::ResourceInfo const * FindResource(Texture::Key const & key,
                                             TextureManager::TextureNode & node) const
  {
    for (size_t i = 0; i < m_textures.size(); ++i)
    {
      RefPointer<Texture> texture = m_textures[i].GetRefPointer();
      Texture::ResourceInfo const * info =  texture->FindResource(key);
      if (info != NULL)
      {
        node.m_width = texture->GetWidth();
        node.m_height = texture->GetHeight();
        node.m_textureOffset = i;
        return info;
      }
    }

    return NULL;
  }

  void UpdateDynamicTextures()
  {
    for_each(m_textures.begin(), m_textures.end(), bind(&Texture::UpdateState,
                                                        bind(&NonConstGetter<Texture>, _1)));
  }

  void BindTextureSet() const
  {
    for (size_t i = 0; i < m_textures.size(); ++i)
    {
      GLFunctions::glActiveTexture(gl_const::GLTexture0 + i);
      m_textures[i]->Bind();
    }
  }

  uint32_t GetSize() const
  {
    return m_textures.size();
  }

private:
  vector<MasterPointer<Texture> > m_textures;
  uint32_t m_maxSize;
};

int TextureManager::GetMaxTextureSet() const
{
  return m_textures.size();
}

void TextureManager::UpdateDynamicTextures()
{
  for_each(m_textures.begin(), m_textures.end(), bind(&TextureSet::UpdateDynamicTextures,
                                                        bind(&NonConstGetter<TextureSet>, _1)));
}

void TextureManager::Init(string const & resourcePrefix)
{
  // in shader we handle only 8 textures
  m_maxTextureBlocks = min(8, GLFunctions::glGetInteger(gl_const::GLMaxFragmentTextures));
  SymbolsTexture * symbols = new SymbolsTexture();
  symbols->Load(my::JoinFoldersToPath(string("resources-") + resourcePrefix, "symbols"));

  TextureSet * defaultSet = new TextureSet(m_maxTextureBlocks);
  defaultSet->AddTexture(MovePointer<Texture>(symbols));

  m_textures.push_back(MasterPointer<TextureSet>(defaultSet));

  vector<TransferPointer<Texture> > tempTextures;
  LoadFont(string("resources/font"), tempTextures);
  for (size_t i = 0; i < tempTextures.size(); ++i)
  {
    RefPointer<TextureSet> set = m_textures.back().GetRefPointer();
    if (set->IsFull())
    {
      m_textures.push_back(MasterPointer<TextureSet>(new TextureSet(m_maxTextureBlocks)));
      set = m_textures.back().GetRefPointer();
    }

    set->AddTexture(tempTextures[i]);
  }

  RefPointer<TextureSet> textureSet = m_textures.back().GetRefPointer();
  if (textureSet->IsFull())
  {
    m_textures.push_back(MasterPointer<TextureSet>(new TextureSet(m_maxTextureBlocks)));
    textureSet = m_textures.back().GetRefPointer();
  }

  typedef DynamicTexture<StipplePenIndex, StipplePenKey, Texture::StipplePen> TStippleTexture;
  textureSet->AddTexture(MovePointer<Texture>(new TStippleTexture(m2::PointU(1024, 1024), dp::ALPHA)));
}

void TextureManager::Release()
{
  DeleteRange(m_textures, MasterPointerDeleter());
}

void TextureManager::GetSymbolRegion(string const & symbolName, SymbolRegion & region) const
{
  SymbolsTexture::SymbolKey key(symbolName);
  TextureNode node;
  node.m_textureSet = 0;
  Texture::ResourceInfo const * info = m_textures[0]->FindResource(key, node);
  ASSERT(node.m_textureOffset != -1, ());
  region.SetResourceInfo(info);
  region.SetTextureNode(node);
}

template <typename TKey, typename TRegion>
bool TextureManager::FindResource(TKey const & key, TRegion & region) const
{
  for (size_t i = 0; i < m_textures.size(); ++i)
  {
    TextureNode node;
    Texture::ResourceInfo const * info = m_textures[i]->FindResource(key, node);
    if (info != NULL)
    {
      node.m_textureSet = i;
      region.SetTextureNode(node);
      region.SetResourceInfo(info);
      return true;
    }
  }

  return false;
}

bool TextureManager::GetGlyphRegion(strings::UniChar charCode, GlyphRegion & region) const
{
  FontTexture::GlyphKey key(charCode);
  return FindResource(key, region);
}

void TextureManager::GetStippleRegion(StipplePenKey const & pen, TextureSetHolder::StippleRegion & region) const
{
  VERIFY(FindResource(pen, region), ());
}

void TextureManager::BindTextureSet(uint32_t textureSet) const
{
  ASSERT_LESS(textureSet, m_textures.size(), ());
  m_textures[textureSet]->BindTextureSet();
}

uint32_t TextureManager::GetTextureCount(uint32_t textureSet) const
{
  ASSERT_LESS(textureSet, m_textures.size(), ());
  return m_textures[textureSet]->GetSize();
}


TextureSetBinder::TextureSetBinder(RefPointer<TextureManager> manager)
  : m_manager(manager)
{
}

void TextureSetBinder::BindTextureSet(uint32_t textureSet) const
{
  m_manager->BindTextureSet(textureSet);
}

uint32_t TextureSetBinder::GetTextureCount(uint32_t textureSet) const
{
  return m_manager->GetTextureCount(textureSet);
}

} // namespace dp
