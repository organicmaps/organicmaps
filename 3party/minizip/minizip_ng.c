// Single translation unit bundling the minizip-ng sources that Organic Maps
// compiles. It lets the iOS/macOS Xcode build -- which cannot run minizip-ng's
// CMake -- reference one file instead of a dozen, and keeps the CMake and Xcode
// builds on a single source list.
//
// Only ZIP read/write with DEFLATE is enabled; the required feature defines
// (HAVE_ZLIB, ZLIB_COMPAT, MZ_ZIP_NO_CRYPTO, MZ_ZIP_NO_ENCRYPTION) and the
// generated mz_config.h are provided by the build (see
// 3party/minizip/CMakeLists.txt and xcode/minizip).
//
// Paths are repo-root-relative; nested "mz.h" etc. resolve next to each source.

// Feature selection, shared by the CMake and Xcode builds: ZIP read/write with
// DEFLATE via zlib(-ng), no encryption and no other codecs. These must precede
// the includes below.
#define HAVE_ZLIB             // Use zlib(-ng) for DEFLATE.
#define ZLIB_COMPAT           // Linked zlib exposes the classic API (z_stream/deflate).
#define MZ_ZIP_NO_CRYPTO      // No signing/hashing crypto backend.
#define MZ_ZIP_NO_ENCRYPTION  // No PKWARE / WinZIP-AES encryption.

#include "3party/minizip-ng/mz_crypt.c"
#include "3party/minizip-ng/mz_os.c"
#include "3party/minizip-ng/mz_strm.c"
#include "3party/minizip-ng/mz_strm_buf.c"
#include "3party/minizip-ng/mz_strm_mem.c"
#include "3party/minizip-ng/mz_strm_split.c"
#include "3party/minizip-ng/mz_strm_zlib.c"
#include "3party/minizip-ng/mz_zip.c"
#include "3party/minizip-ng/mz_zip_rw.c"

#if defined(_WIN32)
#include "3party/minizip-ng/mz_os_win32.c"
#include "3party/minizip-ng/mz_strm_os_win32.c"
#else
#include "3party/minizip-ng/mz_os_posix.c"
#include "3party/minizip-ng/mz_strm_os_posix.c"
#endif

// Classic unzOpen/zipOpen API used by the C++ wrapper (minizip.cpp).
#include "3party/minizip-ng/compat/ioapi.c"
#include "3party/minizip-ng/compat/unzip.c"
#include "3party/minizip-ng/compat/zip.c"
