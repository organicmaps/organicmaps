#include "testing/testing.hpp"

#include "coding/huffman.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/string_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

namespace
{
vector<strings::UniString> MakeUniStringVector(vector<string> const & v)
{
  vector<strings::UniString> result(v.size());
  for (size_t i = 0; i < v.size(); ++i)
    result[i] = strings::MakeUniString(v[i]);
  return result;
}

void TestDecode(coding::HuffmanCoder const & h, uint32_t bits, uint32_t len, uint32_t expected)
{
  coding::HuffmanCoder::Code code(bits, len);
  uint32_t received;
  TEST(h.Decode(code, received), ("Could not decode", code.bits, "( length", code.len, ")"));
  TEST_EQUAL(expected, received, ());
}
}  // namespace

namespace coding
{
UNIT_TEST(Huffman_Smoke)
{
  HuffmanCoder h;
  h.Init(MakeUniStringVector(vector<string>{"ab", "ac"}));

  TestDecode(h, 0, 1, static_cast<uint32_t>('a'));  // 0
  TestDecode(h, 1, 2, static_cast<uint32_t>('b'));  // 10
  TestDecode(h, 3, 2, static_cast<uint32_t>('c'));  // 11
}

UNIT_TEST(Huffman_OneSymbol)
{
  HuffmanCoder h;
  h.Init(MakeUniStringVector(vector<string>{string(5, 0)}));

  TestDecode(h, 0, 0, 0);
}

UNIT_TEST(Huffman_NonAscii)
{
  HuffmanCoder h;
  string const data = "2πΩ";
  strings::UniString const uniData = strings::MakeUniString(data);
  h.Init(vector<strings::UniString>{uniData});

  TestDecode(h, 0, 2, static_cast<uint32_t>(uniData[0]));  // 00
  TestDecode(h, 1, 1, static_cast<uint32_t>(uniData[1]));  // 1
  TestDecode(h, 2, 2, static_cast<uint32_t>(uniData[2]));  // 01
}

UNIT_TEST(Huffman_Init)
{
  HuffmanCoder h;
  h.Init(MakeUniStringVector(vector<string>{"ab"}));

  vector<uint8_t> buf;
  buf.push_back(16);   // size
  buf.push_back(105);  // 01101001
  buf.push_back(150);  // 10010110

  MemReader memReader(&buf[0], buf.size());
  ReaderSource<MemReader> reader(memReader);
  strings::UniString received = h.ReadAndDecode(reader);
  strings::UniString expected = strings::MakeUniString("baababbaabbabaab");

  TEST_EQUAL(expected, received, ());
}

UNIT_TEST(Huffman_Serialization_Encoding)
{
  HuffmanCoder hW;
  hW.Init(MakeUniStringVector(vector<string>{"aaaaaaaaaa", "bbbbbbbbbb", "ccccc", "ddddd"}));  // 10, 10, 5, 5
  vector<uint8_t> buf;
  MemWriter<vector<uint8_t>> writer(buf);
  hW.WriteEncoding(writer);

  HuffmanCoder hR;
  MemReader memReader(&buf[0], buf.size());
  ReaderSource<MemReader> reader(memReader);
  hR.ReadEncoding(reader);

  TEST_EQUAL(reader.Pos(), writer.Pos(), ());

  TestDecode(hW, 0, 2, static_cast<uint32_t>('a'));  // 00
  TestDecode(hW, 2, 2, static_cast<uint32_t>('b'));  // 01
  TestDecode(hW, 1, 2, static_cast<uint32_t>('c'));  // 10
  TestDecode(hW, 3, 2, static_cast<uint32_t>('d'));  // 11

  TestDecode(hR, 0, 2, static_cast<uint32_t>('a'));
  TestDecode(hR, 2, 2, static_cast<uint32_t>('b'));
  TestDecode(hR, 1, 2, static_cast<uint32_t>('c'));
  TestDecode(hR, 3, 2, static_cast<uint32_t>('d'));
}

UNIT_TEST(Huffman_Serialization_Data)
{
  HuffmanCoder hW;
  hW.Init(MakeUniStringVector(vector<string>{"aaaaaaaaaa", "bbbbbbbbbb", "ccccc", "ddddd"}));  // 10, 10, 5, 5
  vector<uint8_t> buf;

  string const data = "abacabaddddaaabbcabacabadbabd";
  strings::UniString expected = strings::UniString(data.begin(), data.end());

  MemWriter<vector<uint8_t>> writer(buf);
  hW.WriteEncoding(writer);
  hW.EncodeAndWrite(writer, expected);

  HuffmanCoder hR;
  MemReader memReader(&buf[0], buf.size());
  ReaderSource<MemReader> reader(memReader);
  hR.ReadEncoding(reader);
  strings::UniString received = hR.ReadAndDecode(reader);

  TEST_EQUAL(expected, received, ());
}

}  // namespace coding
