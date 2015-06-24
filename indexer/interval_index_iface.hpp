#pragma once

#include "std/cstdint.hpp"
#include "std/function.hpp"


class IntervalIndexIFace
{
public:
  virtual ~IntervalIndexIFace() {}

  typedef function<void (uint32_t)> FunctionT;

  virtual void DoForEach(FunctionT const & f, uint64_t beg, uint64_t end) = 0;
};
