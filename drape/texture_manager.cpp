#include "texture_manager.hpp"

#include "../coding/file_name_utils.hpp"

#include "../platform/platform.hpp"

void TextureManager::LoadExternalResources(const string & resourcePostfix)
{
  m_symbols.Load(my::JoinFoldersToPath(string("resources-") + resourcePostfix, "symbols"));
}

void TextureManager::GetSymbolRect(const string & symbolName, TextureManager::Symbol & symbol) const
{
  symbol.m_textureBlock = 0;
  symbol.m_texCoord = m_symbols.FindSymbol(symbolName);
}
