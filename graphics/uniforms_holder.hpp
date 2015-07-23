#pragma once

#include "graphics/defines.hpp"
#include "geometry/point2d.hpp"
#include "base/matrix.hpp"
#include "std/array.hpp"
#include "std/map.hpp"

namespace graphics
{
  class UniformsHolder
  {
  public:
    bool insertValue(ESemantic sem, float value);
    bool insertValue(ESemantic sem, float x, float y);
    bool insertValue(ESemantic sem, float x, float y, float z, float w);
    bool insertValue(ESemantic sem, math::Matrix<float, 4, 4> const & matrix);

    bool getValue(ESemantic sem, float & value) const;
    bool getValue(ESemantic sem, float & x, float & y) const;
    bool getValue(ESemantic sem, float & x, float & y, float & z, float & w) const;
    bool getValue(ESemantic sem, math::Matrix<float, 4, 4> & value) const;

  private:
    template<typename THolder> using THolderMap = map<ESemantic, THolder>;
    using TFloatMap = THolderMap<float>;
    using TVec2Map = THolderMap<m2::PointF>;
    using TVec4Map = THolderMap<array<float, 4>>;
    using TMat4Map = THolderMap<math::Matrix<float, 4, 4>>;

    template<typename THolder>
    bool insertValue(THolderMap<THolder> & holder, ESemantic sem, THolder const & value)
    {
      return holder.insert(make_pair(sem, value)).second;
    }

    template<typename THolder>
    bool getValue(THolderMap<THolder> const & holder, ESemantic sem, THolder & value) const
    {
      typename THolderMap<THolder>::const_iterator it = holder.find(sem);
      if (it == holder.end())
        return false;

      value = it->second;
      return true;
    }

    TFloatMap m_floatHolder;
    TVec2Map m_vec2Holder;
    TVec4Map m_vec4Holder;
    TMat4Map m_mat4Holder;
  };
}

