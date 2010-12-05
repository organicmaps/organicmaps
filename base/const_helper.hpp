#pragma once

namespace my
{
  template <typename TFrom, typename TTo> struct PropagateConst
  {
    typedef TTo type;
  };
  template <typename TFrom, typename TTo> struct PropagateConst<TFrom const, TTo>
  {
    typedef TTo const type;
  };
}
