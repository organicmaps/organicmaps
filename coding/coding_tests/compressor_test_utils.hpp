#pragma once
#include "testing/testing.hpp"

#include "std/cstring.hpp"


namespace coding
{
  inline void TestCompressor(char const * pSrc, size_t srcSize, string & res)
  {
    res = "<";
    res.insert(res.end(), pSrc, pSrc + srcSize);
    res.insert(res.end(), '>');
  }

  inline void TestDecompressor(char const * pSrc, size_t srcSize, char * pDst, size_t dstSize)
  {
    TEST_GREATER_OR_EQUAL(srcSize, 2, ());
    TEST_EQUAL(srcSize - 2, dstSize, ());
    TEST_EQUAL(pSrc[0], '<', ());
    TEST_EQUAL(pSrc[srcSize-1], '>', ());
    memcpy(pDst, pSrc + 1, srcSize - 2);
  }
}
