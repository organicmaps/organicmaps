#include "../../testing/testing.hpp"

#include "../internal/file_data.hpp"
#include "../writer.hpp"

#include "../../base/logging.hpp"


UNIT_TEST(FileData_Api_Smoke)
{
  string name = "test.file";
  string newName = "new_test.file";

  try
  {
    // just create file and close it immediately
    my::FileData f(name, my::FileData::OP_WRITE_TRUNCATE);
  }
  catch (Writer::OpenException const &)
  {
    LOG(LCRITICAL, ("Can't create test file"));
    return;
  }

  uint64_t sz;
  TEST_EQUAL(my::GetFileSize(name, sz), true, ());
  TEST_EQUAL(sz, 0, ());

  TEST_EQUAL(my::RenameFileX(name, newName), true, ());

  TEST_EQUAL(my::GetFileSize(name, sz), false, ());
  TEST_EQUAL(my::GetFileSize(newName, sz), true, ());
  TEST_EQUAL(sz, 0, ());

  TEST_EQUAL(my::DeleteFileX(newName), true, ());

  TEST_EQUAL(my::GetFileSize(newName, sz), false, ());
}

/*
UNIT_TEST(FileData_NoDiskSpace)
{
  char const * name = "/Volumes/KINDLE/file.bin";
  vector<uint8_t> bytes(100000000);

  try
  {
    my::FileData f(name, my::FileData::OP_WRITE_TRUNCATE);

    for (size_t i = 0; i < 100; ++i)
      f.Write(&bytes[0], bytes.size());
  }
  catch (Writer::Exception const & ex)
  {
    LOG(LINFO, ("Writer exception catched"));
  }

  (void)my::DeleteFileX(name);
}
*/
