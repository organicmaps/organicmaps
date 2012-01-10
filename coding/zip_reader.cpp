#include "zip_reader.hpp"

#include "../base/scope_guard.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../3party/zlib/contrib/minizip/unzip.h"

ZipFileReader::ZipFileReader(string const & container, string const & file)
  : BaseZipFileReaderType(container)
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
    MYTHROW(LocateZipException, ("Can't get uncompressed file size inside zip", file));

  if (fileInfo.compressed_size != fileInfo.uncompressed_size)
    MYTHROW(InvalidZipException, ("File should be uncompressed inside zip", file));

  LOG(LDEBUG, (file, "offset:", offset, "size:", fileInfo.uncompressed_size));

  SetOffsetAndSize(offset, fileInfo.uncompressed_size);
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
