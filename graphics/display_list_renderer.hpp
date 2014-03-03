#pragma once

#include "../std/map.hpp"

#include "opengl/storage.hpp"
#include "opengl/base_texture.hpp"
#include "opengl/geometry_renderer.hpp"

namespace graphics
{
  class DisplayList;

  class DisplayListRenderer : public gl::GeometryRenderer
  {
  private:

    DisplayList * m_displayList;

  public:

    typedef gl::GeometryRenderer base_t;
    typedef base_t::Params Params;

    typedef base_t::FreeTexture FreeTextureCmd;
    typedef base_t::FreeStorage FreeStorageCmd;
    typedef base_t::DiscardStorage DiscardStorageCmd;

    typedef gl::BaseTexture const * TextureRef;
    typedef pair<gl::BufferObject const *, gl::BufferObject const *> StorageRef;

    typedef map<TextureRef, pair<int, shared_ptr<FreeTextureCmd> > > DelayedFreeTextureMap;
    typedef map<StorageRef, pair<int, shared_ptr<FreeStorageCmd> > > DelayedFreeStorageMap;
    typedef map<StorageRef, pair<int, shared_ptr<DiscardStorageCmd> > > DelayedDiscardStorageMap;

    DelayedFreeTextureMap m_freeTextureCmds;
    DelayedFreeStorageMap m_freeStorageCmds;
    DelayedDiscardStorageMap m_discardStorageCmds;

    void addStorageRef(StorageRef const & storage);
    void removeStorageRef(StorageRef const & storage);

    void addTextureRef(TextureRef const & texture);
    void removeTextureRef(TextureRef const & texture);

    DisplayListRenderer(Params const & p);

    /// create display list
    DisplayList * createDisplayList();
    /// set current display list
    void setDisplayList(DisplayList * displayList);
    /// get current display list
    DisplayList * displayList() const;
    /// draw display list
    void drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m,
                         UniformsHolder * holder);

    /// Interceptable commands
    /// @{

    /// draw geometry
    void drawGeometry(shared_ptr<gl::BaseTexture> const & texture,
                      gl::Storage const & storage,
                      size_t indicesCount,
                      size_t indicesOffs,
                      EPrimitives primType);
    /// upload ResourceStyle's on texture
    void uploadResources(shared_ptr<Resource> const * resources,
                         size_t count,
                         shared_ptr<gl::BaseTexture> const & texture);
    /// free texture
    void freeTexture(shared_ptr<gl::BaseTexture> const & texture,
                     TTexturePool * texturePool);
    /// free storage
    void freeStorage(gl::Storage const & storage,
                     TStoragePool * storagePool);
    /// unlock storage
    void unlockStorage(gl::Storage const & storage);
    /// discard storage
    void discardStorage(gl::Storage const & storage);
    /// add checkpoint
    void addCheckPoint();
    /// apply blit states
    void applyBlitStates();
    /// apply geometry rendering states
    void applyStates();
    void applyVarAlfaStates();
    /// apply sharp geometry states
    void applySharpStates();
    /// @}
  };
}
