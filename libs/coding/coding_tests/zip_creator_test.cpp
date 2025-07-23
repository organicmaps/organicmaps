#include "testing/testing.hpp"

#include "coding/constants.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/zip_creator.hpp"
#include "coding/zip_reader.hpp"

#include "base/scope_guard.hpp"

#include <string>
#include <vector>

namespace
{
void CreateAndTestZip(std::string const & filePath, std::string const & zipPath)
{
  TEST(CreateZipFromFiles({filePath}, zipPath, CompressionLevel::DefaultCompression), ());

  ZipFileReader::FileList files;
  ZipFileReader::FilesList(zipPath, files);
  TEST_EQUAL(files[0].second, FileReader(filePath).Size(), ());

  std::string const unzippedFile = "unzipped.tmp";
  ZipFileReader::UnzipFile(zipPath, files[0].first, unzippedFile);

  TEST(base::IsEqualFiles(filePath, unzippedFile), ());
  TEST(base::DeleteFileX(filePath), ());
  TEST(base::DeleteFileX(zipPath), ());
  TEST(base::DeleteFileX(unzippedFile), ());
}

void CreateAndTestZip(std::vector<std::string> const & files, std::string const & zipPath, CompressionLevel compression)
{
  TEST(CreateZipFromFiles(files, zipPath, compression), ());

  ZipFileReader::FileList fileList;
  ZipFileReader::FilesList(zipPath, fileList);
  std::string const unzippedFile = "unzipped.tmp";
  for (size_t i = 0; i < files.size(); ++i)
  {
    TEST_EQUAL(fileList[i].second, FileReader(files[i]).Size(), ());

    ZipFileReader::UnzipFile(zipPath, fileList[i].first, unzippedFile);

    TEST(base::IsEqualFiles(files[i], unzippedFile), ());
    TEST(base::DeleteFileX(unzippedFile), ());
  }
  TEST(base::DeleteFileX(zipPath), ());
}

void CreateAndTestZipWithFolder(std::vector<std::string> const & files, std::vector<std::string> const & filesInArchive,
                                std::string const & zipPath, CompressionLevel compression)
{
  TEST(CreateZipFromFiles(files, filesInArchive, zipPath, compression), ());

  ZipFileReader::FileList fileList;
  ZipFileReader::FilesList(zipPath, fileList);
  std::string const unzippedFile = "unzipped.tmp";
  for (size_t i = 0; i < files.size(); ++i)
  {
    TEST_EQUAL(fileList[i].second, FileReader(files[i]).Size(), ());

    ZipFileReader::UnzipFile(zipPath, fileList[i].first, unzippedFile);

    TEST(base::IsEqualFiles(files[i], unzippedFile), ());
    TEST(base::DeleteFileX(unzippedFile), ());
  }
  TEST(base::DeleteFileX(zipPath), ());
}

std::vector<CompressionLevel> GetCompressionLevels()
{
  return {CompressionLevel::DefaultCompression, CompressionLevel::BestCompression, CompressionLevel::BestSpeed,
          CompressionLevel::NoCompression};
}
}  // namespace

UNIT_TEST(CreateZip_BigFile)
{
  std::string const name = "testfileforzip.txt";

  {
    FileWriter f(name);
    std::string s(READ_FILE_BUFFER_SIZE + 1, '1');
    f.Write(s.c_str(), s.size());
  }

  CreateAndTestZip(name, "testzip.zip");
}

UNIT_TEST(CreateZip_Smoke)
{
  std::string const name = "testfileforzip.txt";

  {
    FileWriter f(name);
    f.Write(name.c_str(), name.size());
  }

  CreateAndTestZip(name, "testzip.zip");
}

UNIT_TEST(CreateZip_MultipleFiles)
{
  std::vector<std::string> const fileData{"testf1", "testfile2", "testfile3_longname.txt.xml.csv"};
  SCOPE_GUARD(deleteFileGuard, [&fileData]()
  {
    for (auto const & file : fileData)
      TEST(base::DeleteFileX(file), ());
  });

  for (auto const & name : fileData)
  {
    FileWriter f(name);
    f.Write(name.c_str(), name.size());
  }

  for (auto compression : GetCompressionLevels())
    CreateAndTestZip(fileData, "testzip.zip", compression);
}

UNIT_TEST(CreateZip_MultipleFilesWithFolders)
{
  std::vector<std::string> const fileData{"testf1", "testfile2", "testfile3_longname.txt.xml.csv"};
  std::vector<std::string> const fileInArchiveData{"testf1", "f2/testfile2", "f3/testfile3_longname.txt.xml.csv"};
  SCOPE_GUARD(deleteFileGuard, [&fileData]()
  {
    for (auto const & file : fileData)
      TEST(base::DeleteFileX(file), ());
  });

  for (auto const & name : fileData)
  {
    FileWriter f(name);
    f.Write(name.c_str(), name.size());
  }

  for (auto compression : GetCompressionLevels())
    CreateAndTestZipWithFolder(fileData, fileInArchiveData, "testzip.zip", compression);
}

UNIT_TEST(CreateZip_MultipleFilesSingleEmpty)
{
  std::vector<std::string> const fileData{"singleEmptyfile.txt"};
  SCOPE_GUARD(deleteFileGuard, [&fileData]() { TEST(base::DeleteFileX(fileData[0]), ()); });

  {
    FileWriter f(fileData[0]);
  }

  for (auto compression : GetCompressionLevels())
    CreateAndTestZip(fileData, "testzip.zip", compression);
}
