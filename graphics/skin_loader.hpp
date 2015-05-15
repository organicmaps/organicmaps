/// @author Siarhei Rachytski
#pragma once

#include "geometry/rect2d.hpp"

#include "std/function.hpp"
#include "std/list.hpp"
#include "std/shared_ptr.hpp"

namespace graphics
{
  class SkinLoader
  {
  private:

    enum EMode
    {
      ERoot,
      EPage,
      ESkin,
      EIcon,
      EResource
    };

    list<EMode> m_mode;

    /// resourceStyle-specific parameters
    int32_t m_id;
    uint32_t m_texX;
    uint32_t m_texY;
    uint32_t m_texWidth;
    uint32_t m_texHeight;

    /// pointStyle-specific parameters
    string m_resID;

    /// skin-page specific parameters
    string m_fileName;

  public:

    /// @param _1 - symbol rect on texture. Pixel rect
    /// @param _2 - symbol name. Key on resource search
    /// @param _3 - unique id
    /// @param _4 - texture file name
    using TIconCallback = function<void (m2::RectU const &, string const &, int32_t, string const &)>;

    SkinLoader(TIconCallback const & callback);

    bool Push(string const & element);
    void Pop(string const & element);
    void AddAttr(string const & attribute, string const & value);
    void CharData(string const &) {}

    void popIcon();

  private:
    TIconCallback m_callback;
  };
}
