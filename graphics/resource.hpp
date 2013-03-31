#pragma once

#include "../geometry/rect2d.hpp"

#include "../std/shared_ptr.hpp"

namespace graphics
{
  struct Resource
  {
    enum Category
    {
      EBrush = 1,
      EPen,
      EGlyph,
      EIcon,
      ECircle,
      EImage,
      EUnknown
    };

    /// Base class for lighweight Resource description
    struct Info
    {
      Category m_category;

      Info(Category cat);

      virtual Info const & cacheKey() const = 0;
      /// returns the size of this resource info which will
      /// be occupied in texture cache.
      virtual m2::PointU const resourceSize() const = 0;
      /// factory method for Resource object
      virtual Resource * createResource(m2::RectU const & texRect,
                                        uint8_t pipelineID) const = 0;
      /// comparing for using Info as a key in map.
      virtual bool lessThan(Info const * r) const = 0;
    };

    Category m_cat;
    m2::RectU m_texRect;
    int m_pipelineID;

    virtual ~Resource();
    virtual void render(void * dst) = 0;
    /// get key for ResourceCache.
    virtual Info const * info() const = 0;

    struct LessThan
    {
       bool operator()(Info const * l,
                       Info const * r) const;
    };

  protected:
    Resource(Category cat,
             m2::RectU const & texRect,
             int pipelineID);
  };
}
