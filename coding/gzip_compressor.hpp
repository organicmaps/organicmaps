#pragma once
#include "coding/coder.hpp"
#include "base/base.hpp"
#include "base/exception.hpp"

DECLARE_EXCEPTION(CompressGZipException, StringCodingException);
DECLARE_EXCEPTION(DecompressGZipException, StringCodingException);

// Throws CompressGZipException on error.
void CompressGZip(int level, char const * pSrc, size_t srcSize, string & dst);

// Throws DecompressGZipException on error.
void DecompressGZip(char const * pSrc, size_t srcSize, string & dst);

// Returns -1 if dstSize is too small, otherwise the size of pDst used.
// Throws DecompressGZipException on error.
size_t DecompressGZipIntoFixedSize(char const * pSrc, size_t srcSize, char * pDst, size_t dstSize);
