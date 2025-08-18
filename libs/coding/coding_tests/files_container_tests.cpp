#include "testing/testing.hpp"

#include "coding/files_container.hpp"
#include "coding/varint.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#ifndef OMIM_OS_WINDOWS
#include <unistd.h>  // _SC_PAGESIZE
#endif

using namespace std;

UNIT_TEST(FilesContainer_Smoke)
{
  string const fName = "files_container.tmp";
  FileWriter::DeleteFileX(fName);
  size_t const count = 10;

  // fill container one by one
  {
    FilesContainerW writer(fName);

    for (size_t i = 0; i < count; ++i)
    {
      auto w = writer.GetWriter(strings::to_string(i));

      for (uint32_t j = 0; j < i; ++j)
        WriteVarUint(w, j);
    }
  }

  // read container one by one
  {
    FilesContainerR reader(fName);

    for (size_t i = 0; i < count; ++i)
    {
      FilesContainerR::TReader r = reader.GetReader(strings::to_string(i));
      ReaderSource<FilesContainerR::TReader> src(r);

      for (uint32_t j = 0; j < i; ++j)
      {
        uint32_t const test = ReadVarUint<uint32_t>(src);
        TEST_EQUAL(j, test, ());
      }
    }
  }

  // append to container
  uint32_t const arrAppend[] = {888, 777, 666};
  for (size_t i = 0; i < ARRAY_SIZE(arrAppend); ++i)
  {
    {
      FilesContainerW writer(fName, FileWriter::OP_WRITE_EXISTING);

      auto w = writer.GetWriter(strings::to_string(arrAppend[i]));
      WriteVarUint(w, arrAppend[i]);
    }

    // read appended
    {
      FilesContainerR reader(fName);

      FilesContainerR::TReader r = reader.GetReader(strings::to_string(arrAppend[i]));
      ReaderSource<FilesContainerR::TReader> src(r);

      uint32_t const test = ReadVarUint<uint32_t>(src);
      TEST_EQUAL(arrAppend[i], test, ());
    }
  }
  FileWriter::DeleteFileX(fName);
}

namespace
{
void CheckInvariant(FilesContainerR & reader, string const & tag, int64_t test)
{
  FilesContainerR::TReader r = reader.GetReader(tag);
  TEST_EQUAL(test, ReadPrimitiveFromPos<int64_t>(r, 0), ());
}
}  // namespace

UNIT_TEST(FilesContainer_Shared)
{
  string const fName = "files_container.tmp";
  FileWriter::DeleteFileX(fName);

  uint32_t const count = 10;
  int64_t const test64 = 908175281437210836LL;

  {
    // shared container fill

    FilesContainerW writer(fName);

    auto w1 = writer.GetWriter("5");
    WriteToSink(w1, uint32_t(0));

    for (uint32_t i = 0; i < count; ++i)
      WriteVarUint(w1, i);
    w1->Flush();

    auto w2 = writer.GetWriter("2");
    WriteToSink(w2, test64);
    w2->Flush();
  }

  {
    // shared container read and fill

    FilesContainerR reader(fName);
    FilesContainerR::TReader r1 = reader.GetReader("5");
    uint64_t const offset = sizeof(uint32_t);
    r1 = r1.SubReader(offset, r1.Size() - offset);

    CheckInvariant(reader, "2", test64);

    FilesContainerW writer(fName, FileWriter::OP_WRITE_EXISTING);
    auto w = writer.GetWriter("3");

    ReaderSource<FilesContainerR::TReader> src(r1);
    for (uint32_t i = 0; i < count; ++i)
    {
      uint32_t test = ReadVarUint<uint32_t>(src);
      TEST_EQUAL(test, i, ());
      WriteVarUint(w, i);
    }
  }

  {
    // check invariant
    FilesContainerR reader(fName);
    CheckInvariant(reader, "2", test64);
  }

  FileWriter::DeleteFileX(fName);
}

namespace
{
void ReplaceInContainer(string const & fName, char const * key, char const * value)
{
  FilesContainerW writer(fName, FileWriter::OP_WRITE_EXISTING);
  auto w = writer.GetWriter(key);
  w->Write(value, strlen(value));
}

void CheckContainer(string const & fName, char const * key[], char const * value[], size_t count)
{
  FilesContainerR reader(fName);
  LOG(LINFO, ("Size=", reader.GetFileSize()));

  for (size_t i = 0; i < count; ++i)
  {
    FilesContainerR::TReader r = reader.GetReader(key[i]);

    size_t const szBuffer = 100;
    size_t const szS = strlen(value[i]);

    char s[szBuffer] = {0};
    ASSERT_LESS(szS, szBuffer, ());
    r.Read(0, s, szS);

    TEST(strcmp(value[i], s) == 0, (s));
  }
}
}  // namespace

UNIT_TEST(FilesContainer_RewriteExisting)
{
  string const fName = "files_container.tmp";
  FileWriter::DeleteFileX(fName);

  char const * key[] = {"3", "2", "1"};
  char const * value[] = {"prolog", "data", "epilog"};

  // fill container
  {
    FilesContainerW writer(fName);

    for (size_t i = 0; i < ARRAY_SIZE(key); ++i)
    {
      auto w = writer.GetWriter(key[i]);
      w->Write(value[i], strlen(value[i]));
    }
  }

  // re-write middle file in container
  char const * buffer1 = "xxxxxxx";
  ReplaceInContainer(fName, key[1], buffer1);
  char const * value1[] = {value[0], buffer1, value[2]};
  CheckContainer(fName, key, value1, 3);

  // re-write end file in container
  char const * buffer2 = "yyyyyyyyyyyyyy";
  ReplaceInContainer(fName, key[2], buffer2);
  char const * value2[] = {value[0], buffer1, buffer2};
  CheckContainer(fName, key, value2, 3);

  // re-write end file in container once again
  char const * buffer3 = "zzz";
  ReplaceInContainer(fName, key[2], buffer3);
  char const * value3[] = {value[0], buffer1, buffer3};
  CheckContainer(fName, key, value3, 3);

  FileWriter::DeleteFileX(fName);
}

/// @todo To make this test work, need to review FilesContainerW::GetWriter logic.
/*
UNIT_TEST(FilesContainer_ConsecutiveRewriteExisting)
{
  string const fName = "files_container.tmp";
  FileWriter::DeleteFileX(fName);

  char const * key[] = { "3", "2", "1" };
  char const * value[] = { "prolog", "data", "epilog" };

  // fill container
  {
    FilesContainerW writer(fName);

    for (size_t i = 0; i < ARRAY_SIZE(key); ++i)
    {
      auto w = writer.GetWriter(key[i]);
      w->Write(value[i], strlen(value[i]));
    }
  }

  char const * buf0 = "xxx";
  char const * buf1 = "yyy";
  {
    FilesContainerW writer(fName, FileWriter::OP_WRITE_EXISTING);

    {
      auto w = writer.GetWriter(key[0]);
      w->Write(buf0, strlen(buf0));
    }

    {
      auto w = writer.GetWriter(key[1]);
      w->Write(buf1, strlen(buf1));
    }
  }

  char const * values[] = { buf0, buf1, value[2] };
  CheckContainer(fName, key, values, 3);
}
*/

UNIT_TEST(FilesMappingContainer_Handle)
{
  string const fName = "files_container.tmp";
  string const tag = "dummy";

  {
    FilesContainerW writer(fName);
    auto w = writer.GetWriter(tag);
    w->Write(tag.c_str(), tag.size());
  }

  {
    FilesMappingContainer cont(fName);

    FilesMappingContainer::Handle h1 = cont.Map(tag);
    TEST(h1.IsValid(), ());

    FilesMappingContainer::Handle h2;
    TEST(!h2.IsValid(), ());

    h2.Assign(std::move(h1));
    TEST(!h1.IsValid(), ());
    TEST(h2.IsValid(), ());
  }

  FileWriter::DeleteFileX(fName);
}

UNIT_TEST(FilesMappingContainer_MoveHandle)
{
  static uint8_t const kNumMapTests = 200;
  class HandleWrapper
  {
  public:
    explicit HandleWrapper(FilesMappingContainer::Handle && handle) : m_handle(std::move(handle))
    {
      TEST(m_handle.IsValid(), ());
    }

  private:
    FilesMappingContainer::Handle m_handle;
  };

  string const containerPath = "files_container.tmp";
  string const tagName = "dummy";

  SCOPE_GUARD(deleteContainerFileGuard, bind(&FileWriter::DeleteFileX, cref(containerPath)));

  {
    FilesContainerW writer(containerPath);
    auto w = writer.GetWriter(tagName);
    w->Write(tagName.c_str(), tagName.size());
  }

  {
    FilesMappingContainer cont(containerPath);

    FilesMappingContainer::Handle h1 = cont.Map(tagName);
    TEST(h1.IsValid(), ());

    FilesMappingContainer::Handle h2(std::move(h1));
    TEST(h2.IsValid(), ());
    TEST(!h1.IsValid(), ());

    for (int i = 0; i < kNumMapTests; ++i)
    {
      FilesMappingContainer::Handle parent_handle = cont.Map(tagName);
      HandleWrapper tmp(std::move(parent_handle));
    }
  }
}

UNIT_TEST(FilesMappingContainer_Smoke)
{
  string const fName = "files_container.tmp";
  char const * key[] = {"3", "2", "1"};
  uint32_t const count = 1000000;

  // fill container
  {
    FilesContainerW writer(fName);

    for (size_t i = 0; i < ARRAY_SIZE(key); ++i)
    {
      auto w = writer.GetWriter(key[i]);
      for (uint32_t j = 0; j < count; ++j)
      {
        uint32_t v = j + static_cast<uint32_t>(i);
        w->Write(&v, sizeof(v));
      }
    }
  }

  {
    FilesMappingContainer reader(fName);

    for (size_t i = 0; i < ARRAY_SIZE(key); ++i)
    {
      FilesMappingContainer::Handle h = reader.Map(key[i]);
      uint32_t const * data = h.GetData<uint32_t>();

      for (uint32_t j = 0; j < count; ++j)
      {
        TEST_EQUAL(j + i, *data, ());
        ++data;
      }

      h.Unmap();
    }
  }

  FileWriter::DeleteFileX(fName);
}

UNIT_TEST(FilesMappingContainer_PageSize)
{
  string const fName = "files_container.tmp";

  size_t const pageSize =
#ifndef OMIM_OS_WINDOWS
      sysconf(_SC_PAGESIZE);
#else
      4096;
#endif
  LOG(LINFO, ("Page size:", pageSize));

  char const * key[] = {"3", "2", "1"};
  char const byte[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};
  size_t count[] = {pageSize - 1, pageSize, pageSize + 1};
  size_t const sz = ARRAY_SIZE(key);

  {
    FilesContainerW writer(fName);

    for (size_t i = 0; i < sz; ++i)
    {
      auto w = writer.GetWriter(key[i]);
      for (size_t j = 0; j < count[i]; ++j)
        w->Write(&byte[j % ARRAY_SIZE(byte)], 1);
    }
  }

  {
    FilesMappingContainer reader(fName);
    FilesMappingContainer::Handle handle[sz];

    for (size_t i = 0; i < sz; ++i)
    {
      handle[i].Assign(reader.Map(key[i]));
      TEST_EQUAL(handle[i].GetSize(), count[i], ());
    }

    for (size_t i = 0; i < sz; ++i)
    {
      char const * data = handle[i].GetData<char>();
      for (size_t j = 0; j < count[i]; ++j)
        TEST_EQUAL(*data++, byte[j % ARRAY_SIZE(byte)], ());
    }
  }

  FileWriter::DeleteFileX(fName);
}
