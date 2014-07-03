#include "texture_manager.hpp"
#include "symbols_texture.hpp"

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

void TextureManager::Init(string const & resourcePostfix)
{
  // in shader we handle only 8 textures
  m_maxTextureBlocks = min(8, GLFunctions::glGetInteger(gl_const::GLMaxFragmentTextures));
  SymbolsTexture * symbols = new SymbolsTexture();
  symbols->Load(my::JoinFoldersToPath(string("resources-") + resourcePostfix, "symbols"));

  m_textures.Reset(new TextureSet(m_maxTextureBlocks));
  m_textures->AddTexture(MovePointer<Texture>(symbols));
}

void TextureManager::Release()
{
  m_textures.Destroy();
}

void TextureManager::GetSymbolRegion(string const & symbolName, SymbolRegion & region) const
{
  SymbolsTexture::SymbolKey key(symbolName);
  TextureNode node;
  node.m_textureSet = 0;
  Texture::ResourceInfo const * info = m_textures->FindResource(key, node);
  ASSERT(node.m_textureOffset != -1, ());
  region.SetResourceInfo(info);
  region.SetTextureNode(node);
}

void TextureManager::GetGlyphRegion(strings::UniChar charCode, GlyphRegion & region) const
{
  // todo;
}

void TextureManager::BindTextureSet(uint32_t textureSet) const
{
  ASSERT_LESS(textureSet, 1, ()); // TODO replace 1 to m_textureSets.size()
  m_textures->BindTextureSet();
}

uint32_t TextureManager::GetTextureCount(uint32_t textureSet) const
{
  ASSERT_LESS(textureSet, 1, ()); // TODO replace 1 to m_textureSets.size()
  return m_textures->GetSize();
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
