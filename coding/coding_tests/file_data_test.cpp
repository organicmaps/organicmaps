#include "testing/testing.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

namespace
{
  string const name1 = "test1.file";
  string const name2 = "test2.file";

  void MakeFile(string const & name)
  {
    base::FileData f(name, base::FileData::OP_WRITE_TRUNCATE);
    f.Write(name.c_str(), name.size());
  }

  void MakeFile(string const & name, size_t const size, const char c)
  {
    base::FileData f(name, base::FileData::OP_WRITE_TRUNCATE);
    f.Write(string(size, c).c_str(), size);
  }

#ifdef OMIM_OS_WINDOWS
  void CheckFileOK(string const & name)
  {
    base::FileData f(name, base::FileData::OP_READ);

    uint64_t const size = f.Size();
    TEST_EQUAL ( size, name.size(), () );

    vector<char> buffer(size);
    f.Read(0, &buffer[0], size);
    TEST ( equal(name.begin(), name.end(), buffer.begin()), () );
  }
#endif
}

UNIT_TEST(FileData_ApiSmoke)
{
  MakeFile(name1);
  uint64_t const size = name1.size();

  uint64_t sz;
  TEST(base::GetFileSize(name1, sz), ());
  TEST_EQUAL(sz, size, ());

  TEST(base::RenameFileX(name1, name2), ());

  TEST(!base::GetFileSize(name1, sz), ());
  TEST(base::GetFileSize(name2, sz), ());
  TEST_EQUAL(sz, size, ());

  TEST(base::DeleteFileX(name2), ());

  TEST(!base::GetFileSize(name2, sz), ());
}

/*
UNIT_TEST(FileData_NoDiskSpace)
{
  char const * name = "/Volumes/KINDLE/file.bin";
  vector<uint8_t> bytes(100000000);

  try
  {
    base::FileData f(name, base::FileData::OP_WRITE_TRUNCATE);

    for (size_t i = 0; i < 100; ++i)
      f.Write(&bytes[0], bytes.size());
  }
  catch (Writer::Exception const & ex)
  {
    LOG(LINFO, ("Writer exception catched"));
  }

  (void)base::DeleteFileX(name);
}
*/

/*
#ifdef OMIM_OS_WINDOWS
UNIT_TEST(FileData_SharingAV_Windows)
{
  {
    MakeFile(name1);

    // lock file, will check sharing access
    base::FileData f1(name1, base::FileData::OP_READ);

    // try rename or delete locked file
    TEST(!base::RenameFileX(name1, name2), ());
    TEST(!base::DeleteFileX(name1), ());

    MakeFile(name2);

    // try rename or copy to locked file
    TEST(!base::RenameFileX(name2, name1), ());
    TEST(!base::CopyFileX(name2, name1), ());

    // files should be unchanged
    CheckFileOK(name1);
    CheckFileOK(name2);

    //TEST(base::CopyFile(name1, name2), ());
  }

  // renaming to existing file is not allowed
  TEST(!base::RenameFileX(name1, name2), ());
  TEST(!base::RenameFileX(name2, name1), ());

  TEST(base::DeleteFileX(name1), ());
  TEST(base::DeleteFileX(name2), ());
}
#endif
*/

UNIT_TEST(Equal_Function_Test)
{
  MakeFile(name1);
  MakeFile(name2);
  TEST(base::IsEqualFiles(name1, name1), ());
  TEST(base::IsEqualFiles(name2, name2), ());
  TEST(!base::IsEqualFiles(name1, name2), ());

  TEST(base::DeleteFileX(name1), ());
  TEST(base::DeleteFileX(name2), ());
}

UNIT_TEST(Equal_Function_Test_For_Big_Files)
{
  {
    MakeFile(name1, 1024 * 1024, 'a');
    MakeFile(name2, 1024 * 1024, 'a');
    TEST(base::IsEqualFiles(name1, name2), ());
    TEST(base::DeleteFileX(name1), ());
    TEST(base::DeleteFileX(name2), ());
  }
  {
    MakeFile(name1, 1024 * 1024 + 512, 'a');
    MakeFile(name2, 1024 * 1024 + 512, 'a');
    TEST(base::IsEqualFiles(name1, name2), ());
    TEST(base::DeleteFileX(name1), ());
    TEST(base::DeleteFileX(name2), ());
  }
  {
    MakeFile(name1, 1024 * 1024 + 1, 'a');
    MakeFile(name2, 1024 * 1024 + 1, 'b');
    TEST(base::IsEqualFiles(name1, name1), ());
    TEST(base::IsEqualFiles(name2, name2), ());
    TEST(!base::IsEqualFiles(name1, name2), ());
    TEST(base::DeleteFileX(name1), ());
    TEST(base::DeleteFileX(name2), ());
  }
  {
    MakeFile(name1, 1024 * 1024, 'a');
    MakeFile(name2, 1024 * 1024, 'b');
    TEST(base::IsEqualFiles(name1, name1), ());
    TEST(base::IsEqualFiles(name2, name2), ());
    TEST(!base::IsEqualFiles(name1, name2), ());
    TEST(base::DeleteFileX(name1), ());
    TEST(base::DeleteFileX(name2), ());
  }
  {
    MakeFile(name1, 1024 * 1024, 'a');
    MakeFile(name2, 1024 * 1024 + 1, 'b');
    TEST(!base::IsEqualFiles(name1, name2), ());
    TEST(base::DeleteFileX(name1), ());
    TEST(base::DeleteFileX(name2), ());
  }
}
