#pragma once
#include "../std/function.hpp"


class IntervalIndexIFace
{
public:
  virtual ~IntervalIndexIFace() {}

  class QueryIFace
  {
  public:
    virtual ~QueryIFace() {}
  };

  typedef function<void (uint32_t)> FunctionT;

  virtual void DoForEach(FunctionT const & f, uint64_t beg, uint64_t end, QueryIFace & query) = 0;
};
