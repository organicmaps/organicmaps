#include "testing/testing.hpp"

#include "3party/minizip/minizip.hpp"

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

unzip::FileInfo GetFirstZipFileInfo(std::string const & zipPath)
{
  auto zip = unzip::Open(zipPath);
  TEST(zip != nullptr, ());
  SCOPE_GUARD(zipGuard, [&zip]() { unzip::Close(zip); });

  TEST(unzip::GoToFirstFile(zip) == unzip::Code::Ok, ());
  unzip::FileInfo info;
  TEST(unzip::GetCurrentFileInfo(zip, info) == unzip::Code::Ok, ());
  return info;
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

UNIT_TEST(CreateZip_CompressionLevel)
{
  std::string const filePath = "compression_level_source.txt";
  std::string const noCompressionZip = "compression_level_no_compression.zip";
  std::string const bestCompressionZip = "compression_level_best_compression.zip";
  SCOPE_GUARD(deleteFileGuard, [&]()
  {
    TEST(base::DeleteFileX(filePath), ());
    TEST(base::DeleteFileX(noCompressionZip), ());
    TEST(base::DeleteFileX(bestCompressionZip), ());
  });

  std::string const data(64 * 1024, 'a');
  {
    FileWriter f(filePath);
    f.Write(data.c_str(), data.size());
  }

  TEST(CreateZipFromFiles({filePath}, noCompressionZip, CompressionLevel::NoCompression), ());
  TEST(CreateZipFromFiles({filePath}, bestCompressionZip, CompressionLevel::BestCompression), ());

  auto const noCompressionInfo = GetFirstZipFileInfo(noCompressionZip);
  auto const bestCompressionInfo = GetFirstZipFileInfo(bestCompressionZip);
  TEST_EQUAL(noCompressionInfo.m_info.uncompressed_size, data.size(), ());
  TEST_EQUAL(bestCompressionInfo.m_info.uncompressed_size, data.size(), ());
  TEST_LESS(bestCompressionInfo.m_info.compressed_size, noCompressionInfo.m_info.compressed_size, ());
}
