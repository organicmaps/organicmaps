#include "display_list_renderer.hpp"
#include "display_list.hpp"

namespace graphics
{
  DisplayListRenderer::DisplayListRenderer(Params const & p)
    : base_t(p),
      m_displayList(0)
  {
  }

  DisplayList * DisplayListRenderer::createDisplayList()
  {
    return new DisplayList(this);
  }

  void DisplayListRenderer::setDisplayList(DisplayList * dl)
  {
    m_displayList = dl;
  }

  DisplayList * DisplayListRenderer::displayList() const
  {
    return m_displayList;
  }

  void DisplayListRenderer::drawDisplayList(DisplayList * dl,
                                            math::Matrix<double, 3, 3> const & m)
  {
    dl->draw(this, m);
  }

  void DisplayListRenderer::drawGeometry(shared_ptr<gl::BaseTexture> const & texture,
                                         gl::Storage const & storage,
                                         size_t indicesCount,
                                         size_t indicesOffs,
                                         EPrimitives primType)
  {
    if (isCancelled())
      return;

    if (m_displayList)
    {
      shared_ptr<DrawGeometry> command(new DrawGeometry());

      command->m_texture = texture;
      command->m_storage = storage;
      command->m_indicesCount = indicesCount;
      command->m_indicesOffs = indicesOffs;
      command->m_primitiveType = primType;

      m_displayList->drawGeometry(command);
    }
    else
      base_t::drawGeometry(texture,
                           storage,
                           indicesCount,
                           indicesOffs,
                           primType);

  }

  void DisplayListRenderer::uploadStyles(shared_ptr<ResourceStyle> const * styles,
                                         size_t count,
                                         shared_ptr<gl::BaseTexture> const & texture)
  {
    if (isCancelled())
      return;

    if (m_displayList)
      m_displayList->uploadStyles(make_shared_ptr(new UploadData(styles, count, texture)));
    else
      base_t::uploadStyles(styles, count, texture);
  }

  void DisplayListRenderer::freeTexture(shared_ptr<gl::BaseTexture> const & texture,
                                        TTexturePool * texturePool)
  {
    if (m_displayList)
    {
      shared_ptr<FreeTexture> command(new FreeTexture());

      command->m_texture = texture;
      command->m_texturePool = texturePool;

      m_displayList->freeTexture(command);
    }
    else
      base_t::freeTexture(texture, texturePool);
  }

  void DisplayListRenderer::freeStorage(gl::Storage const & storage,
                                        TStoragePool * storagePool)
  {
    if (m_displayList)
    {
      shared_ptr<FreeStorage> command(new FreeStorage());

      command->m_storage = storage;
      command->m_storagePool = storagePool;

      m_displayList->freeStorage(command);
    }
    else
      base_t::freeStorage(storage, storagePool);
  }

  void DisplayListRenderer::unlockStorage(gl::Storage const & storage)
  {
    if (m_displayList)
    {
      shared_ptr<UnlockStorage> cmd(new UnlockStorage());

      cmd->m_storage = storage;

      m_displayList->unlockStorage(cmd);
    }
    else
      base_t::unlockStorage(storage);
  }

  void DisplayListRenderer::discardStorage(gl::Storage const & storage)
  {
    if (m_displayList)
    {
      shared_ptr<DiscardStorage> cmd(new DiscardStorage());

      cmd->m_storage = storage;

      m_displayList->discardStorage(cmd);
    }
    else
      base_t::discardStorage(storage);
  }

  void DisplayListRenderer::applyBlitStates()
  {
    if (m_displayList)
      m_displayList->applyBlitStates(make_shared_ptr(new ApplyBlitStates()));
    else
      base_t::applyBlitStates();
  }

  void DisplayListRenderer::applyStates()
  {
    if (m_displayList)
      m_displayList->applyStates(make_shared_ptr(new ApplyStates()));
    else
      base_t::applyStates();
  }

  void DisplayListRenderer::applySharpStates()
  {
    if (m_displayList)
      m_displayList->applySharpStates(make_shared_ptr(new ApplySharpStates()));
    else
      base_t::applySharpStates();
  }

  void DisplayListRenderer::addCheckPoint()
  {
    if (m_displayList)
      m_displayList->addCheckPoint();
    else
      base_t::addCheckPoint();
  }

}
