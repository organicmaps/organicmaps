#include "zip_reader.hpp"

#include "../base/scope_guard.hpp"
#include "../base/logging.hpp"

#include "../coding/file_writer.hpp"

#include "../std/bind.hpp"

#include "../3party/zlib/contrib/minizip/unzip.h"

ZipFileReader::ZipFileReader(string const & container, string const & file)
  : BaseZipFileReaderType(container), m_uncompressedFileSize(0)
{
  unzFile zip = unzOpen64(container.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", container));

  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzLocateFile(zip, file.c_str(), 1))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", file));

  if (UNZ_OK != unzOpenCurrentFile(zip))
      MYTHROW(LocateZipException, ("Can't open file inside zip", file));

  uint64_t offset = unzGetCurrentFileZStreamPos64(zip);
  unzCloseCurrentFile(zip);

  if (offset > Size())
    MYTHROW(LocateZipException, ("Invalid offset inside zip", file));

  unz_file_info64 fileInfo;
  if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get compressed file size inside zip", file));

  SetOffsetAndSize(offset, fileInfo.compressed_size);
  m_uncompressedFileSize = fileInfo.uncompressed_size;
}

vector<string> ZipFileReader::FilesList(string const & zipContainer)
{
  unzFile zip = unzOpen64(zipContainer.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", zipContainer));

  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzGoToFirstFile(zip))
    MYTHROW(LocateZipException, ("Can't find first file inside zip", zipContainer));

  vector<string> filesList;
  do
  {
    char fileName[256];
    if (UNZ_OK != unzGetCurrentFileInfo64(zip, NULL, fileName, ARRAY_SIZE(fileName), NULL, 0, NULL, 0))
      MYTHROW(LocateZipException, ("Can't get file name inside zip", zipContainer));

    filesList.push_back(fileName);

  } while (UNZ_OK == unzGoToNextFile(zip));

  return filesList;
}

bool ZipFileReader::IsZip(string const & zipContainer)
{
  unzFile zip = unzOpen64(zipContainer.c_str());
  if (!zip)
    return false;
  unzClose(zip);
  return true;
}

void ZipFileReader::UnzipFile(string const & zipContainer, string const & fileInZip,
                              string const & outFilePath, ProgressFn progressFn)
{
  unzFile zip = unzOpen64(zipContainer.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", zipContainer));
  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzLocateFile(zip, fileInZip.c_str(), 1))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", fileInZip));

  if (UNZ_OK != unzOpenCurrentFile(zip))
      MYTHROW(LocateZipException, ("Can't open file inside zip", fileInZip));

  unz_file_info64 fileInfo;
  if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get uncompressed file size inside zip", fileInZip));

  MY_SCOPE_GUARD(currentFileGuard, bind(&unzCloseCurrentFile, zip));

  try
  {
    FileWriter outFile(outFilePath);

    int pos = 0;
    int readBytes;
    static size_t const BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    while (true)
    {
      readBytes = unzReadCurrentFile(zip, buf, BUF_SIZE);
      if (readBytes > 0)
        outFile.Write(buf, static_cast<size_t>(readBytes));
      else if (readBytes < 0)
        MYTHROW(InvalidZipException, ("Error", readBytes, "while unzipping", fileInZip, "from", zipContainer));
      else
        break;

      pos += readBytes;

      if (progressFn)
        progressFn(fileInfo.uncompressed_size, pos);
    }
  }
  catch (Exception const & e)
  {
    // Delete unfinished output file
    FileWriter::DeleteFileX(outFilePath);
    // Rethrow exception - we've failed
    throw;
  }
}
