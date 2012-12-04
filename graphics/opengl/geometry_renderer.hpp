#pragma once

#include "../defines.hpp"
#include "../resource_cache.hpp"

#include "clipper.hpp"

#include "../../base/threaded_list.hpp"

#include "../../std/shared_ptr.hpp"
#include "../../std/function.hpp"

namespace graphics
{
  struct ResourceStyle;

  namespace gl
  {
    class BaseTexture;

    class GeometryRenderer : public Clipper
    {
    public:

      typedef Clipper base_t;

      struct UploadData : public Command
      {
        vector<shared_ptr<ResourceStyle> > m_uploadQueue;
        shared_ptr<BaseTexture> m_texture;

        UploadData();
        UploadData(shared_ptr<ResourceStyle> const * styles,
                   size_t count,
                   shared_ptr<BaseTexture> const & texture);

        void perform();
        void cancel();
        void dump();
      };

      struct DrawGeometry : Command
      {
        shared_ptr<BaseTexture> m_texture;
        Storage m_storage;
        size_t m_indicesCount;
        size_t m_indicesOffs;
        EPrimitives m_primitiveType;

        void perform();
        void dump();
      };

      struct IMMDrawTexturedPrimitives : Command
      {
        buffer_vector<m2::PointF, 8> m_pts;
        buffer_vector<m2::PointF, 8> m_texPts;
        unsigned m_ptsCount;
        shared_ptr<BaseTexture> m_texture;
        bool m_hasTexture;
        graphics::Color m_color;
        bool m_hasColor;
        shared_ptr<ResourceManager> m_resourceManager;

        void perform();
      };

      struct IMMDrawTexturedRect : IMMDrawTexturedPrimitives
      {
        IMMDrawTexturedRect(m2::RectF const & rect,
                            m2::RectF const & texRect,
                            shared_ptr<BaseTexture> const & texture,
                            shared_ptr<ResourceManager> const & rm);
      };

      struct FreeStorage : public Command
      {
        TStoragePool * m_storagePool;
        Storage m_storage;

        void perform();
        void cancel();
      };

      struct FreeTexture : public Command
      {
        TTexturePool * m_texturePool;
        shared_ptr<BaseTexture> m_texture;

        void perform();
        void cancel();
        void dump();
      };

      struct UnlockStorage : public Command
      {
        Storage m_storage;

        void perform();
        void cancel();
      };

      struct DiscardStorage : public Command
      {
        Storage m_storage;

        void perform();
        void cancel();
      };

      struct ApplySharpStates : public Command
      {
        void perform();
      };

      struct ApplyStates : public Command
      {
        void perform();
      };

      struct ApplyBlitStates : public Command
      {
        void perform();
      };

      GeometryRenderer(base_t::Params const & params);

      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        Storage const & storage,
                        size_t indicesCount,
                        size_t indicesOffs,
                        EPrimitives primType);

      void uploadStyles(shared_ptr<ResourceStyle> const * styles,
                        size_t count,
                        shared_ptr<BaseTexture> const & texture);

      void freeTexture(shared_ptr<BaseTexture> const & texture, TTexturePool * texturePool);
      void freeStorage(Storage const & storage, TStoragePool * storagePool);
      void unlockStorage(Storage const & storage);
      void discardStorage(Storage const & storage);

      void applySharpStates();
      void applyBlitStates();
      void applyStates();
    };
  }
}

