#include "texture_manager.hpp"
#include "symbols_texture.hpp"
#include "font_texture.hpp"

#include "glfunctions.hpp"

#include "../platform/platform.hpp"

#include "../coding/file_name_utils.hpp"

#include "../base/stl_add.hpp"

#include "../std/vector.hpp"

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
  LoadFont(string("resources"), tempTextures);
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

bool TextureManager::GetGlyphRegion(strings::UniChar charCode, GlyphRegion & region) const
{
  FontTexture::GlyphKey key(charCode);
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
