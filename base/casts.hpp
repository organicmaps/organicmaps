#pragma once
#include "assert.hpp"

template <typename ToT, typename FromT> ToT implicit_cast(FromT const & t)
{
  return t;
}
