#include "testing/testing.hpp"

/*
#include "coding/blob_storage.hpp"
#include "coding/blob_indexer.hpp"

#include "coding/coding_tests/compressor_test_utils.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace
{

string GetBlob(BlobStorage const & bs, uint32_t i)
{
  string blob;
  bs.GetBlob(i, blob);
  return blob;
}

}  // unnamed namespace

UNIT_TEST(BlobIndexerEmptyTest)
{
  string serial;
  {
    MemWriter<string> writer(serial);
    BlobIndexer indexer(writer, 20, &coding::TestCompressor);
  }
  char const expected[] = "Blb\x14\0\0\0\0";
  TEST_EQUAL(serial, string(&expected[0], &expected[ARRAY_SIZE(expected)-1]), ());
  BlobStorage storage(new MemReader(&serial[0], serial.size()), &coding::TestDecompressor);
}

UNIT_TEST(BlobIndexerSimpleSerialTest)
{
  string serial;
  {
    MemWriter<string> writer(serial);
    BlobIndexer indexer(writer, 20, &coding::TestCompressor);
    indexer.AddBlob("abc");
  }
  char const expected[] = "Blb\x14"        // Header
                          "<abc>\3\0\0\0"  // Chunk 0 with its decompressed size
                          "\x9\0\0\0"      // Chunk 0 end offset
                          "\0\0\0\0"       // Blob 0 info
                          "\1\0\0\0";      // Number of chunks
  TEST_EQUAL(serial, string(&expected[0], &expected[ARRAY_SIZE(expected)-1]), ());
  BlobStorage bs(new MemReader(&serial[0], serial.size()), &coding::TestDecompressor);
  TEST_EQUAL(bs.Size(), 1, ());
  TEST_EQUAL(GetBlob(bs, 0), "abc", ());
}

UNIT_TEST(BlobIndexerSerialTest)
{
  string serial;
  {
    MemWriter<string> writer(serial);
    BlobIndexer indexer(writer, 5, &coding::TestCompressor);
    indexer.AddBlob("abc");         // Chunk 0
    indexer.AddBlob("d");           // Chunk 0
    indexer.AddBlob("ef");          // Chunk 1
    indexer.AddBlob("1234567890");  // Chunk 2
    indexer.AddBlob("0987654321");  // Chunk 3
    indexer.AddBlob("Hello");       // Chunk 4
    indexer.AddBlob("World");       // Chunk 5
    indexer.AddBlob("!");           // Chunk 6
  }
  char const expected[] = "Blb\x14"                      // Header
                          "<abcd>\x4\0\0\0"             // Chunk 0
                          "<ef>\x2\0\0\0"               // Chunk 1
                          "<1234567890>\xA\0\0\0"       // Chunk 2
                          "<0987654321>\xA\0\0\0"       // Chunk 3
                          "<Hello>\x5\0\0\0"            // Chunk 4
                          "<World>\x5\0\0\0"            // Chunk 5
                          "<!>\x1\0\0\0"                // Chunk 6
                          "\x0A\0\0\0"                  // Chunk 0 end pos
                          "\x12\0\0\0"                  // Chunk 1 end pos
                          "\x22\0\0\0"                  // Chunk 2 end pos
                          "\x32\0\0\0"                  // Chunk 3 end pos
                          "\x3D\0\0\0"                  // Chunk 4 end pos
                          "\x48\0\0\0"                  // Chunk 5 end pos
                          "\x4F\0\0\0"                  // Chunk 6 end pos
                          "\x0\0\x00\0"                 // Blob 0 info
                          "\x3\0\x00\0"                 // Blob 1 info
                          "\x0\0\x10\0"                 // Blob 2 info
                          "\x0\0\x20\0"                 // Blob 3 info
                          "\x0\0\x30\0"                 // Blob 4 info
                          "\x0\0\x40\0"                 // Blob 5 info
                          "\x0\0\x50\0"                 // Blob 6 info
                          "\x0\0\x60\0"                 // Blob 7 info
                          "\x8\0\0\0"                   // Number of blobs
                          ;
  TEST_EQUAL(serial, string(&expected[0], ARRAY_SIZE(expected) - 1), ());
  BlobStorage bs(new MemReader(&serial[0], serial.size()), &coding::TestDecompressor);
  TEST_EQUAL(bs.Size(), 8, ());
  TEST_EQUAL(GetBlob(bs, 0), "abc", ());
  TEST_EQUAL(GetBlob(bs, 1), "d", ());
  TEST_EQUAL(GetBlob(bs, 2), "ef", ());
  TEST_EQUAL(GetBlob(bs, 3), "1234567890", ());
  TEST_EQUAL(GetBlob(bs, 4), "0987654321", ());
  TEST_EQUAL(GetBlob(bs, 5), "Hello", ());
  TEST_EQUAL(GetBlob(bs, 6), "World", ());
  TEST_EQUAL(GetBlob(bs, 7), "!", ());
}
*/
