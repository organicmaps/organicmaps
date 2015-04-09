#include "coding/bzip2_compressor.hpp"
#include "coding/coder_util.hpp"

#include "base/assert.hpp"

#include "std/vector.hpp"

#include "3party/bzip2/bzlib.h"

size_t DecompressBZip2IntoFixedSize(char const * pSrc, size_t srcSize, char * pDst, size_t dstSize)
{
  // TODO: Remove unnecessary copying.
  vector<char> src(pSrc, pSrc + srcSize);
  unsigned int dstUsed = static_cast<unsigned int>(dstSize);
  int error = BZ2_bzBuffToBuffDecompress(pDst, &dstUsed, &src[0], static_cast<unsigned int>(srcSize), 0, 0);
  switch (error)
  {
  case BZ_OK:
    return dstUsed;
  case BZ_OUTBUFF_FULL:
    return size_t(-1);
  default:
    MYTHROW(DecompressBZip2Exception, (error, srcSize, dstSize, dstUsed));
  }
}

void DecompressBZip2(char const * pSrc, size_t srcSize, string & dst)
{
  FixedDstSizeCodeToString(&DecompressBZip2IntoFixedSize, pSrc, srcSize, dst);
}

void CompressBZip2(int level, char const * pSrc, size_t srcSize, string & dst)
{
  ASSERT(level >= 1 && level <= 9, (level));
  dst.resize(srcSize + srcSize / 100 + 699);
  unsigned int dstUsed = static_cast<unsigned int>(dst.size());
  // TODO: Remove unnecessary copying.
  vector<char> src(pSrc, pSrc + srcSize);
  int error = BZ2_bzBuffToBuffCompress(&dst[0], &dstUsed, &src[0], static_cast<unsigned int>(srcSize), level, 0, 0);
  if (error == BZ_OK)
    dst.resize(dstUsed);
  else
    MYTHROW(CompressBZip2Exception, (error, srcSize, dst.size(), dstUsed));
}
