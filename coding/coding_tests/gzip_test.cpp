#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "coding/coding_tests/coder_test.hpp"
#include "coding/gzip_compressor.hpp"

namespace
{
  void CompressGZipLevel1(char const * pSrc, size_t srcSize, string & dst)
  {
    return CompressGZip(1, pSrc, srcSize, dst);
  }
  void CompressGZipLevel9(char const * pSrc, size_t srcSize, string & dst)
  {
    return CompressGZip(1, pSrc, srcSize, dst);
  }
}

UNIT_TEST(AaaaGZipCompressionLevel1)
{
  CoderAaaaTest(&CompressGZipLevel1, &DecompressGZip);
}

UNIT_TEST(AaaaGZipCompressionLevel9)
{
  CoderAaaaTest(&CompressGZipLevel9, &DecompressGZip);
}
