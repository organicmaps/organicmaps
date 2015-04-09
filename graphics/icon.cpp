#include "graphics/icon.hpp"

namespace graphics
{
  Icon::Info::Info()
    : Resource::Info(Resource::EIcon)
  {}

  Icon::Info::Info(string const & name)
    : Resource::Info(Resource::EIcon),
      m_name(name)
  {}

  Resource::Info const & Icon::Info::cacheKey() const
  {
    return *this;
  }

  m2::PointU const Icon::Info::resourceSize() const
  {
    return m2::PointU(0, 0);
  }

  Resource * Icon::Info::createResource(m2::RectU const & texRect,
                                        uint8_t pipelineID) const
  {
    return 0;
  }

  bool Icon::Info::lessThan(Resource::Info const * i) const
  {
    if (m_category != i->m_category)
      return m_category < i->m_category;

    Icon::Info const * ii = static_cast<Icon::Info const*>(i);

    if (m_name != ii->m_name)
      return m_name < ii->m_name;

    return false;
  }


  Icon::Icon(m2::RectU const & texRect,
             int pipelineID,
             Info const & info)
    : Resource(EIcon,
               texRect,
               pipelineID),
      m_info(info)
  {}

  void Icon::render(void *)
  {}

  Resource::Info const * Icon::info() const
  {
    return &m_info;
  }
}
