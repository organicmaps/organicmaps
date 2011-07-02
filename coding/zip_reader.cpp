#include "zip_reader.hpp"

#include "../base/scope_guard.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../3party/zlib/contrib/minizip/unzip.h"

ZipFileReader::ZipFileReader(string const & container, string const & file)
  : base_type(container)
{
  unzFile zip = unzOpen(container.c_str());
  if (!zip)
    MYTHROW(OpenZipException, ("Can't get zip file handle", container));

  MY_SCOPE_GUARD(zipGuard, bind(&unzClose, zip));

  if (UNZ_OK != unzLocateFile(zip, file.c_str(), 1))
    MYTHROW(LocateZipException, ("Can't locate file inside zip", file));

  unz_file_pos filePos;
  if (UNZ_OK != unzGetFilePos(zip, &filePos))
    MYTHROW(LocateZipException, ("Can't locate file offset inside zip", file));

  unz_file_info fileInfo;
  if (UNZ_OK != unzGetCurrentFileInfo(zip, &fileInfo, NULL, 0, NULL, 0, NULL, 0))
    MYTHROW(LocateZipException, ("Can't get uncompressed file size inside zip", file));

  if (fileInfo.compressed_size != fileInfo.uncompressed_size)
    MYTHROW(InvalidZipException, ("File should be uncompressed inside zip", file));

  LOG(LDEBUG, (file, "offset:", filePos.pos_in_zip_directory, "size:", fileInfo.uncompressed_size));

  SetOffsetAndSize(filePos.pos_in_zip_directory, fileInfo.uncompressed_size);
}
