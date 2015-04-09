#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "coding/coding_tests/coder_test.hpp"
#include "coding/bzip2_compressor.hpp"

namespace
{
  void CompressBZip2Level1(char const * pSrc, size_t srcSize, string & dst)
  {
    return CompressBZip2(1, pSrc, srcSize, dst);
  }
  void CompressBZip2Level9(char const * pSrc, size_t srcSize, string & dst)
  {
    return CompressBZip2(1, pSrc, srcSize, dst);
  }
}

UNIT_TEST(AaaaBZip2CompressionLevel1)
{
  CoderAaaaTest(&CompressBZip2Level1, &DecompressBZip2);
}

UNIT_TEST(AaaaBZip2CompressionLevel9)
{
  CoderAaaaTest(&CompressBZip2Level9, &DecompressBZip2);
}
