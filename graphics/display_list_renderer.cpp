#include "graphics/display_list_renderer.hpp"
#include "graphics/display_list.hpp"

namespace graphics
{

  namespace
  {
    template<typename TKey, typename TCommand>
    void InsertFreeCommand(DisplayListRenderer::DelayedCommandMap<TKey, TCommand> & map,
                           TKey const & key, shared_ptr<TCommand> const & command)
    {
      /// Before free resource call we must (and it is) call drawGeometry with this resource
      /// It creates node in DelayedCommandMap with empty FreeCommand and sets counter to 1
      /// or simply increments counter. Here we check that node exists
      auto it = map.find(key);
      ASSERT(it != map.end(), ());
      it->second.second = command;
    }
  }

  DisplayListRenderer::DisplayListRenderer(Params const & p)
    : base_t(p),
      m_displayList(0)
  {
  }

  void DisplayListRenderer::addStorageRef(StorageRef const & storage)
  {
    m_discardStorageCmds[storage].first++;
    m_freeStorageCmds[storage].first++;
  }

  void DisplayListRenderer::removeStorageRef(StorageRef const & storage)
  {
#ifdef DEBUG
    for (auto const & v : m_discardStorageCmds)
    {
      ASSERT(v.second.first > 0, ());
      ASSERT(v.second.second != nullptr, ());
    }

    for (auto const & v : m_freeStorageCmds)
    {
      ASSERT(v.second.first > 0, ());
      ASSERT(v.second.second != nullptr, ());
    }
#endif

    DelayedDiscardStorageMap::iterator dit = m_discardStorageCmds.find(storage);
    ASSERT(dit != m_discardStorageCmds.end(), ());

    pair<int, shared_ptr<DiscardStorageCmd> > & dval = dit->second;
    --dval.first;

    if ((dval.first == 0) && dval.second)
    {
      dval.second->perform();
      m_discardStorageCmds.erase(dit);
    }

    DelayedFreeStorageMap::iterator fit = m_freeStorageCmds.find(storage);
    ASSERT(fit != m_freeStorageCmds.end(), ());

    pair<int, shared_ptr<FreeStorageCmd> > & fval = fit->second;
    --fval.first;

    if ((fval.first == 0) && fval.second)
    {
      fval.second->perform();
      m_freeStorageCmds.erase(fit);
    }
  }

  void DisplayListRenderer::addTextureRef(TextureRef const & texture)
  {
    pair<int, shared_ptr<FreeTextureCmd> > & val = m_freeTextureCmds[texture];
    val.first++;
  }

  void DisplayListRenderer::removeTextureRef(TextureRef const & texture)
  {
    DelayedFreeTextureMap::iterator tit = m_freeTextureCmds.find(texture);
    ASSERT(tit != m_freeTextureCmds.end(), ());

    pair<int, shared_ptr<FreeTextureCmd> > & val = tit->second;
    --val.first;

    if ((val.first == 0) && val.second)
    {
      val.second->perform();
      m_freeTextureCmds.erase(tit);
    }
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

  void DisplayListRenderer::drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m,
                                            UniformsHolder * holder, size_t indicesCount)
  {
    dl->draw(this, m, holder, indicesCount);
  }

  void DisplayListRenderer::clear(Color const & c, bool clearRT, float depth, bool clearDepth)
  {
    if (m_displayList)
      m_displayList->clear(make_shared<ClearCommandCmd>(c, clearRT, depth, clearDepth));
    else
      base_t::clear(c, clearRT, depth, clearDepth);
  }

  void DisplayListRenderer::drawGeometry(shared_ptr<gl::BaseTexture> const & texture,
                                         gl::Storage const & storage,
                                         size_t indicesCount,
                                         size_t indicesOffs,
                                         EPrimitives primType)
  {
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

  void DisplayListRenderer::drawRouteGeometry(shared_ptr<gl::BaseTexture> const & texture,
                                              gl::Storage const & storage)
  {
    if (m_displayList)
    {
      shared_ptr<DrawRouteGeometry> command(new DrawRouteGeometry());

      command->m_texture = texture;
      command->m_storage = storage;

      m_displayList->drawRouteGeometry(command);
    }
    else
    {
      base_t::drawRouteGeometry(texture, storage);
    }
  }

  void DisplayListRenderer::uploadResources(shared_ptr<Resource> const * resources,
                                            size_t count,
                                            shared_ptr<gl::BaseTexture> const & texture)
  {
    if (m_displayList)
      m_displayList->uploadResources(make_shared<UploadData>(resources, count, texture));
    else
      base_t::uploadResources(resources, count, texture);
  }

  void DisplayListRenderer::freeTexture(shared_ptr<gl::BaseTexture> const & texture,
                                        TTexturePool * texturePool)
  {
    if (m_displayList)
    {
      shared_ptr<FreeTexture> command(new FreeTexture());

      command->m_texture = texture;
      command->m_texturePool = texturePool;
      TextureRef texRef = texture.get();
      InsertFreeCommand(m_freeTextureCmds, texRef, command);
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

      StorageRef sref(storage.m_vertices.get(), storage.m_indices.get());
      InsertFreeCommand(m_freeStorageCmds, sref, command);
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

      StorageRef sref(storage.m_vertices.get(), storage.m_indices.get());
      InsertFreeCommand(m_discardStorageCmds, sref, cmd);
    }
    else
      base_t::discardStorage(storage);
  }

  void DisplayListRenderer::applyBlitStates()
  {
    if (m_displayList)
      m_displayList->applyBlitStates(make_shared<ApplyBlitStates>());
    else
      base_t::applyBlitStates();
  }

  void DisplayListRenderer::applyStates()
  {
    if (m_displayList)
      m_displayList->applyStates(make_shared<ApplyStates>());
    else
      base_t::applyStates();
  }

  void DisplayListRenderer::applyVarAlfaStates()
  {
    if (m_displayList)
      m_displayList->applyStates(make_shared<ApplyStates>(ApplyStates::AlfaVaringProgram));
    else
      base_t::applyVarAlfaStates();
  }

  void DisplayListRenderer::applySharpStates()
  {
    if (m_displayList)
      m_displayList->applySharpStates(make_shared<ApplySharpStates>());
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
