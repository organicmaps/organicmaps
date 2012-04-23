#pragma once

#include "clipper.hpp"
#include "skin_page.hpp"

#include "../base/threaded_list.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"

namespace yg
{
  struct ResourceStyle;

  namespace gl
  {
    class VertexBuffer;
    class IndexBuffer;
    class BaseTexture;
    class DisplayList;

    class GeometryRenderer : public Clipper
    {
    private:

      DisplayList * m_displayList;

    public:

      typedef Clipper base_t;

      struct UploadData : public Command
      {
        SkinPage::TUploadQueue m_uploadQueue;
        shared_ptr<BaseTexture> m_texture;

        UploadData();
        UploadData(SkinPage::TUploadQueue const & uploadQueue,
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
        unsigned m_primitiveType;

        void perform();
        void dump();
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

      struct ApplyStates : public Command
      {
        void perform();
        void cancel();
      };

      struct ApplyBlitStates : public Command
      {
        void perform();
        void cancel();
      };

      GeometryRenderer(base_t::Params const & params);

      void applyBlitStates();
      void applyStates();

      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        Storage const & storage,
                        size_t indicesCount,
                        size_t indicesOffs,
                        unsigned primType);

      void uploadTexture(SkinPage::TUploadQueue const & uploadQueue, shared_ptr<BaseTexture> const & texture);
      void freeTexture(shared_ptr<BaseTexture> const & texture, TTexturePool * texturePool);
      void freeStorage(Storage const & storage, TStoragePool * storagePool);
      void unlockStorage(Storage const & storage);
      void discardStorage(Storage const & storage);

      /// create display list
      DisplayList * createDisplayList();
      /// set current display list
      void setDisplayList(DisplayList * displayList);
      /// get current display list
      DisplayList * displayList() const;
    };
  }
}

