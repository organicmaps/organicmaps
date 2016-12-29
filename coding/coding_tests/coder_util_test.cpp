#include "testing/testing.hpp"

#include "coding/coder_util.hpp"
#include "base/logging.hpp"

//namespace
//{
//  size_t FixedDstSizeInfiniteMemoryEncode(char const *, size_t, char *, size_t /*dstSize*/)
//  {
//    // LOG(LINFO, ("DstSize", dstSize));
//    return -1U;
//  }
//}

/* Commented because takes too much time after memory upgrage :)
UNIT_TEST(FixedDstSizeCodeToStringInfiniteMemoryTest)
{
  bool thrownDstOutOfMemoryStringCodingException = false;
  try
  {
    char const src [] = "Test";
    string dst;
    FixedDstSizeCodeToString(&FixedDstSizeInfiniteMemoryEncode, src, 4, dst);
  }
  catch (DstOutOfMemoryStringCodingException & e)
  {
    thrownDstOutOfMemoryStringCodingException = true;
  }
  TEST(thrownDstOutOfMemoryStringCodingException, ());
}
*/
