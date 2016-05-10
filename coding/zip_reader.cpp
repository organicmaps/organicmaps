#include "coding/zip_reader.hpp"

#include "base/scope_guard.hpp"
#include "base/logging.hpp"

#include "coding/file_writer.hpp"
#include "coding/constants.hpp"

#include "std/bind.hpp"

#include "3party/minizip/unzip.h"


ZipFileReader::ZipFileReader(string const & container, string const & file,
                             uint32_t logPageSize, uint32_t logPageCount)
  : FileReader(container, logPageSize, logPageCount), m_uncompressedFileSize(0)
{
  unzFile zip = unzOpen64(container.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", container));

  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzLocateFile(zip, file.c_str(), 1))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", file));

  if (UNZ_OK != unzOpenCurrentFile(zip))
    MYTHROW(LocateZipException, ("Can't open file inside zip", file));

  uint64_t const offset = unzGetCurrentFileZStreamPos64(zip);
  (void) unzCloseCurrentFile(zip);

  if (offset == 0 || offset > Size())
    MYTHROW(LocateZipException, ("Invalid offset inside zip", file));

  unz_file_info64 fileInfo;
  if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get compressed file size inside zip", file));

  SetOffsetAndSize(offset, fileInfo.compressed_size);
  m_uncompressedFileSize = fileInfo.uncompressed_size;
}

void ZipFileReader::FilesList(string const & zipContainer, FileListT & filesList)
{
  unzFile const zip = unzOpen64(zipContainer.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", zipContainer));

  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzGoToFirstFile(zip))
    MYTHROW(LocateZipException, ("Can't find first file inside zip", zipContainer));

  do
  {
    char fileName[256];
    unz_file_info64 fileInfo;
    if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, fileName, ARRAY_SIZE(fileName), NULL, 0, NULL, 0))
      MYTHROW(LocateZipException, ("Can't get file name inside zip", zipContainer));

    filesList.push_back(make_pair(fileName, fileInfo.uncompressed_size));

  } while (UNZ_OK == unzGoToNextFile(zip));
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
  MY_SCOPE_GUARD(currentFileGuard, bind(&unzCloseCurrentFile, zip));

  unz_file_info64 fileInfo;
  if (UNZ_OK != unzGetCurrentFileInfo64(zip, &fileInfo, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get uncompressed file size inside zip", fileInZip));

  // First outFile should be closed, then FileWriter::DeleteFileX is called,
  // so make correct order of guards.
  MY_SCOPE_GUARD(outFileGuard, bind(&FileWriter::DeleteFileX, cref(outFilePath)));
  FileWriter outFile(outFilePath);

  uint64_t pos = 0;
  char buf[ZIP_FILE_BUFFER_SIZE];
  while (true)
  {
    int const readBytes = unzReadCurrentFile(zip, buf, ZIP_FILE_BUFFER_SIZE);
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

  outFileGuard.release();
}

void ZipFileReader::UnzipFileToMemory(string const & cont, string const & file,
                                      vector<char> & data)
{
  unzFile zip = unzOpen64(cont.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle:", cont));
  MY_SCOPE_GUARD(zipCloser, bind(&unzClose, zip));

  if (UNZ_OK != unzLocateFile(zip, file.c_str(), 1 /* case sensitivity */))
    MYTHROW(LocateZipException, ("Can't locate file inside zip container:", file));
  if (UNZ_OK != unzOpenCurrentFile(zip))
    MYTHROW(LocateZipException, ("Can't open file inside zip container:", file));
  MY_SCOPE_GUARD(currentFileCloser, bind(&unzCloseCurrentFile, zip));

  unz_file_info64 info;
  if (UNZ_OK != unzGetCurrentFileInfo64(zip, &info, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get uncompressed file size inside zip:", file));

  size_t const size = info.uncompressed_size;
  data.resize(size);

  int const bytesRead = unzReadCurrentFile(zip, data.data(), size);
  if (bytesRead < 0)
    MYTHROW(InvalidZipException, ("Error:", bytesRead, "while unzipping", file, "in", cont));
}
