#pragma once

#include "../defines.hpp"
#include "../resource_cache.hpp"

#include "clipper.hpp"

#include "../../base/threaded_list.hpp"

#include "../../std/shared_ptr.hpp"
#include "../../std/function.hpp"

namespace graphics
{
  struct Resource;

  namespace gl
  {
    class BaseTexture;

    class GeometryRenderer : public Clipper
    {
    public:

      typedef Clipper base_t;

      struct UploadData : public Command
      {
        vector<shared_ptr<Resource> > m_uploadQueue;
        shared_ptr<BaseTexture> m_texture;

        UploadData();
        UploadData(shared_ptr<Resource> const * styles,
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
        VertexDecl * m_vertexDecl;
        shared_ptr<Program> m_program;

        DrawGeometry();
        virtual bool isNeedAdditionalUniforms() const;
        virtual void setAdditionalUniforms(UniformsHolder const & holder);
        void perform();
        void dump();

      private:
        float m_alfa;
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
        enum ProgramType
        {
          DefaultProgram,
          AlfaVaringProgram
        };

        ApplyStates(ProgramType type = DefaultProgram) : m_type(type) {}
        void perform();

      private:
        ProgramType m_type;
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

      void uploadResources(shared_ptr<Resource> const * styles,
                           size_t count,
                           shared_ptr<BaseTexture> const & texture);

      void freeTexture(shared_ptr<BaseTexture> const & texture, TTexturePool * texturePool);
      void freeStorage(Storage const & storage, TStoragePool * storagePool);
      void unlockStorage(Storage const & storage);
      void discardStorage(Storage const & storage);

      void applySharpStates();
      void applyBlitStates();
      void applyVarAlfaStates();
      void applyStates();
    };
  }
}

