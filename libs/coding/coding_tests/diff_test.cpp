#include "testing/testing.hpp"

#include "coding/byte_stream.hpp"
#include "coding/dd_vector.hpp"
#include "coding/diff.hpp"
#include "coding/reader.hpp"

#include "base/rolling_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

UNIT_TEST(MyersSimpleDiff)
{
  vector<char> tmp;
  PushBackByteSink<vector<char>> sink(tmp);
  TEST_EQUAL(4, diff::DiffMyersSimple(string("axxxb"), string("cxxxd"), 5, sink), ());
  TEST_EQUAL(5, diff::DiffMyersSimple(string("abcabba"), string("cbabac"), 10, sink), ());
  TEST_EQUAL(5, diff::DiffMyersSimple(string("abcabba"), string("cbabac"), 5, sink), ());
  TEST_EQUAL(-1, diff::DiffMyersSimple(string("abcabba"), string("cbabac"), 4, sink), ());
  TEST_EQUAL(-1, diff::DiffMyersSimple(string("abcabba"), string("cbabac"), 2, sink), ());
  TEST_EQUAL(-1, diff::DiffMyersSimple(string("abcabba"), string("cbabac"), 1, sink), ());
}

class TestPatchWriter
{
public:
  template <typename IterT>
  void WriteData(IterT it, uint64_t n)
  {
    for (uint64_t i = 0; i < n; ++i, ++it)
      m_Stream << *it;
  }

  void WriteOperation(uint64_t op) { m_Stream << op << "."; }

  string Str() { return m_Stream.str(); }

private:
  ostringstream m_Stream;
};

UNIT_TEST(PatchCoderCopyFirst)
{
  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  patchCoder.Copy(2);
  patchCoder.Copy(1);
  patchCoder.Insert("ab", 2);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "6.ab5.", ());
}

UNIT_TEST(PatchCoderInsertFirst)
{
  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  patchCoder.Insert("abc", 3);
  patchCoder.Copy(3);
  patchCoder.Insert("d", 1);
  patchCoder.Insert("e", 1);
  patchCoder.Delete(5);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "abc7.6.de5.11.", ());
}

UNIT_TEST(PatchCoderDeleteFirst)
{
  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  patchCoder.Delete(3);
  patchCoder.Copy(2);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "6.5.", ());
}

UNIT_TEST(PatchCoderEmptyPatch)
{
  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "", ());
}

// PatchCoder mock.
// Uses simple diff format "=x.-x.+str" where x is number, "." - operation separator, str - string.
// Ignores commands with n == 0, but doesn't merge same commands together, i.e. "=2.=2." won't be
// merged into "=4."
class TestPatchCoder
{
public:
  typedef size_t size_type;

  void Copy(size_t n)
  {
    if (n != 0)
      m_Stream << "=" << n << ".";
  }

  void Delete(size_t n)
  {
    if (n != 0)
      m_Stream << "-" << n << ".";
  }

  template <typename IterT>
  void Insert(IterT it, size_t n)
  {
    if (n == 0)
      return;
    m_Stream << "+";
    for (size_t i = 0; i < n; ++i, ++it)
      m_Stream << *it;
    m_Stream << ".";
  }
  void Finalize() {}
  string Str() { return m_Stream.str(); }

private:
  ostringstream m_Stream;
};

UNIT_TEST(DiffSimpleReplace)
{
  char const src[] = "abcxxxdef";
  char const dst[] = "abcyydef";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::SimpleReplaceDiffer differ;

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "=3.-3.+yy.=3.", ());

  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), patchCoder);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "6.6.yy4.6.", ());
}

UNIT_TEST(DiffSimpleReplaceEmptyBegin)
{
  char const src[] = "xxxdef";
  char const dst[] = "yydef";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::SimpleReplaceDiffer differ;

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "-3.+yy.=3.", ());

  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), patchCoder);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "6.yy4.6.", ());
}

UNIT_TEST(DiffSimpleReplaceEmptyEnd)
{
  char const src[] = "abcxxx";
  char const dst[] = "abcyy";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::SimpleReplaceDiffer differ;

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "=3.-3.+yy.", ());

  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), patchCoder);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "6.6.yy4.", ());
}

UNIT_TEST(DiffSimpleReplaceAllEqual)
{
  char const src[] = "abcdef";
  char const dst[] = "abcdef";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::SimpleReplaceDiffer differ;

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "=6.", ());

  TestPatchWriter patchWriter;
  diff::PatchCoder<TestPatchWriter> patchCoder(patchWriter);
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), patchCoder);
  patchCoder.Finalize();
  TEST_EQUAL(patchWriter.Str(), "12.", ());
}

UNIT_TEST(DiffWithRollingHashEqualStrings)
{
  char const src[] = "abcdefklmno";
  char const dst[] = "abcdefklmno";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::RollingHashDiffer<diff::SimpleReplaceDiffer, RollingHasher64> differ(3);

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "=3.=3.=3.=2.", ());
}

UNIT_TEST(DiffWithRollingHashCompletelyDifferentStrings)
{
  char const src[] = "pqrstuvw";
  char const dst[] = "abcdefgh";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::RollingHashDiffer<diff::SimpleReplaceDiffer, RollingHasher64> differ(3);

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "-8.+abcdefgh.", ());
}

UNIT_TEST(DiffWithRollingHash1)
{
  char const src[] = "abcdefghijklmnop";
  char const dst[] = "abcdfeghikkklmnop";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::RollingHashDiffer<diff::SimpleReplaceDiffer, RollingHasher64> differ(3);

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "=3.=1.-2.+fe.=3.-1.+kk.=2.=3.=1.", ());
}

UNIT_TEST(DiffWithRollingHash2)
{
  char const src[] = "abcdefghijklmnop";
  char const dst[] = "abxdeflmnop";
  MemReader srcReader(src, ARRAY_SIZE(src) - 1);
  MemReader dstReader(dst, ARRAY_SIZE(dst) - 1);
  DDVector<char, MemReader> srcV(srcReader);  // since sizeof(char) == 1
  DDVector<char, MemReader> dstV(dstReader);  // since sizeof(char) == 1

  diff::RollingHashDiffer<diff::SimpleReplaceDiffer, RollingHasher64> differ(3);

  TestPatchCoder testPatchCoder;
  differ.Diff(srcV.begin(), srcV.end(), dstV.begin(), dstV.end(), testPatchCoder);
  TEST_EQUAL(testPatchCoder.Str(), "=2.-1.+x.=3.-5.=1.=3.=1.", ());
}
