#include "base/shared_buffer_manager.hpp"

#include "graphics/opengl/opengl.hpp"
#include "graphics/opengl/managed_texture.hpp"

namespace graphics
{
  namespace gl
  {
    ManagedTexture::ManagedTexture(unsigned width, unsigned height, size_t pixelSize)
      : BaseTexture(width, height), m_imageSize(width * height * pixelSize), m_isLocked(false)
    {}

    ManagedTexture::ManagedTexture(m2::PointU const & size, size_t pixelSize)
      : BaseTexture(size.x, size.y), m_imageSize(size.x * size.y * pixelSize), m_isLocked(false)
    {}

    void ManagedTexture::lock()
    {
      m_isLocked = true;
      m_auxData = SharedBufferManager::instance().reserveSharedBuffer(m_imageSize);
    }

    void ManagedTexture::unlock()
    {
      SharedBufferManager::instance().freeSharedBuffer(m_imageSize, m_auxData);

      m_auxData.reset();

      m_isLocked = false;
    }

    void * ManagedTexture::auxData()
    {
      ASSERT(m_isLocked, ("texture is unlocked"));
      return &(*m_auxData)[0];
    }
  }
}
