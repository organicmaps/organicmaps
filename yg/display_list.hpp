#pragma once

#include "geometry_renderer.hpp"

namespace yg
{
  namespace gl
  {
    class DisplayList
    {
    private:

      typedef GeometryRenderer::DrawGeometry DrawGeometryCmd;
      typedef GeometryRenderer::DiscardStorage DiscardStorageCmd;
      typedef GeometryRenderer::FreeTexture FreeTextureCmd;
      typedef GeometryRenderer::UnlockStorage UnlockStorageCmd;
      typedef GeometryRenderer::FreeStorage FreeStorageCmd;
      typedef GeometryRenderer::ApplyBlitStates ApplyBlitStatesCmd;
      typedef GeometryRenderer::ApplyStates ApplyStatesCmd;
      typedef GeometryRenderer::UploadData UploadDataCmd;

      list<shared_ptr<Command> > m_commands;

      list<shared_ptr<DiscardStorageCmd> > m_discardStorageCmd;
      list<shared_ptr<FreeTextureCmd> > m_freeTextureCmd;
      list<shared_ptr<FreeStorageCmd> > m_freeStorageCmd;

      GeometryRenderer * m_parent;
      bool m_isDebugging;

      friend class GeometryRenderer;
      DisplayList(GeometryRenderer * parent);

    public:

      ~DisplayList();

      void applyStates(shared_ptr<ApplyStatesCmd> const & cmd);
      void applyBlitStates(shared_ptr<ApplyBlitStatesCmd> const & cmd);
      void drawGeometry(shared_ptr<DrawGeometryCmd> const & cmd);
      void unlockStorage(shared_ptr<UnlockStorageCmd> const & cmd);
      void discardStorage(shared_ptr<DiscardStorageCmd> const & cmd);
      void freeTexture(shared_ptr<FreeTextureCmd> const & cmd);
      void freeStorage(shared_ptr<FreeStorageCmd> const & cmd);
      void uploadData(shared_ptr<UploadDataCmd> const & cmd);

      void draw(math::Matrix<double, 3, 3> const & m);
    };
  }
}
