#include "coding/gzip_compressor.hpp"
#include "coding/coder_util.hpp"
#include "base/assert.hpp"
#include "3party/zlib/zlib.h"

size_t DecompressGZipIntoFixedSize(char const * pSrc, size_t srcSize, char * pDst, size_t dstSize)
{
  unsigned long dstUsed = dstSize;
  int error = uncompress(reinterpret_cast<unsigned char *>(pDst), &dstUsed,
                         reinterpret_cast<unsigned char const *>(pSrc), srcSize);
  switch (error)
  {
  case Z_OK:
    return dstUsed;
  case Z_BUF_ERROR:
    return size_t(-1);
  default:
    MYTHROW(DecompressGZipException, (error, srcSize, dstSize, dstUsed));
  }
}

void DecompressGZip(char const * pSrc, size_t srcSize, string & dst)
{
  FixedDstSizeCodeToString(&DecompressGZipIntoFixedSize, pSrc, srcSize, dst);
}

void CompressGZip(int level, char const * pSrc, size_t srcSize, string & dst)
{
  ASSERT(level >= 1 && level <= 9, (level));
  dst.resize(compressBound(srcSize));
  unsigned long dstUsed = dst.size();
  int error = compress2(reinterpret_cast<unsigned char *>(&dst[0]), &dstUsed,
                        reinterpret_cast<unsigned char const *>(pSrc), srcSize, level);
  if (error == Z_OK)
    dst.resize(dstUsed);
  else
    MYTHROW(CompressGZipException, (error, srcSize, dst.size(), dstUsed));
}

