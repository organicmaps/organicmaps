#pragma once

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

    DisplayListRenderer(Params const & p);

    /// create display list
    DisplayList * createDisplayList();
    /// set current display list
    void setDisplayList(DisplayList * displayList);
    /// get current display list
    DisplayList * displayList() const;
    /// draw display list
    void drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m);

    /// Interceptable commands
    /// @{

    /// draw geometry
    void drawGeometry(shared_ptr<gl::BaseTexture> const & texture,
                      gl::Storage const & storage,
                      size_t indicesCount,
                      size_t indicesOffs,
                      EPrimitives primType);
    /// upload ResourceStyle's on texture
    void uploadStyles(shared_ptr<ResourceStyle> const * styles,
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
    /// apply sharp geometry states
    void applySharpStates();
    /// @}
  };
}
