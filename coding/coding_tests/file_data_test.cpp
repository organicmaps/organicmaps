#include "testing/testing.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace file_data_test
{
  std::string const name1 = "test1.file";
  std::string const name2 = "test2.file";

  void MakeFile(std::string const & name)
  {
    base::FileData f(name, base::FileData::OP_WRITE_TRUNCATE);
    f.Write(name.c_str(), name.size());
  }

  void MakeFile(std::string const & name, size_t const size, const char c)
  {
    base::FileData f(name, base::FileData::OP_WRITE_TRUNCATE);
    f.Write(std::string(size, c).c_str(), size);
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

UNIT_TEST(EmptyFile)
{
  using namespace base;

  std::string const name = "test.empty";
  std::string const copy = "test.empty.copy";

  // Check that both files are not exist.
  uint64_t sz;
  TEST(!GetFileSize(name, sz), ());
  TEST(!GetFileSize(copy, sz), ());

  // Try to copy non existing file - failed.
  TEST(!CopyFileX(name, copy), ());

  // Again, both files are not exist.
  TEST(!GetFileSize(name, sz), ());
  TEST(!GetFileSize(copy, sz), ());

  {
    // Create empty file with zero size.
    FileData f(name, base::FileData::OP_WRITE_TRUNCATE);
  }

  // Check that empty file is on disk.
  TEST(GetFileSize(name, sz), ());
  TEST_EQUAL(sz, 0, ());

  // Do copy.
  TEST(CopyFileX(name, copy), ());
  //TEST(!RenameFileX(name, copy), ());

  // Delete copy file and rename name -> copy.
  TEST(DeleteFileX(copy), ());
  TEST(RenameFileX(name, copy), ());

  // Now we don't have an initial file but have a copy.
  TEST(!GetFileSize(name, sz), ());
  TEST(GetFileSize(copy, sz), ());
  TEST_EQUAL(sz, 0, ());

  // Delete copy file.
  TEST(DeleteFileX(copy), ());
}

// Made this 'obvious' test for getline. I had (or not?) behaviour when 'while (getline)' loop
// didn't get last string in file without trailing '\n'.
UNIT_TEST(File_StdGetLine)
{
  std::string const fName = "test.txt";

  for (std::string buffer : { "x\nxy\nxyz\nxyzk", "x\nxy\nxyz\nxyzk\n" })
  {
    {
      base::FileData f(fName, base::FileData::OP_WRITE_TRUNCATE);
      f.Write(buffer.c_str(), buffer.size());
    }

    {
      std::ifstream ifs(fName);
      std::string line;
      size_t count = 0;
      while (std::getline(ifs, line))
      {
        ++count;
        TEST_EQUAL(line.size(), count, ());
      }

      TEST_EQUAL(count, 4, ());
    }

    TEST(base::DeleteFileX(fName), ());
  }
}

} // namespace file_data_test
