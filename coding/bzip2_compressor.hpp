#pragma once
#include "coding/coder.hpp"
#include "base/base.hpp"
#include "base/exception.hpp"

DECLARE_EXCEPTION(CompressBZip2Exception, StringCodingException);
DECLARE_EXCEPTION(DecompressBZip2Exception, StringCodingException);

// Throws CompressBZip2Exception on error.
void CompressBZip2(int level, char const * pSrc, size_t srcSize, string & dst);

// Throws DecompressBZip2Exception on error.
void DecompressBZip2(char const * pSrc, size_t srcSize, string & dst);

// Returns -1 if dstSize is too small, otherwise the size of pDst used.
// Throws DecompressBZip2Exception on error.
size_t DecompressBZip2IntoFixedSize(char const * pSrc, size_t srcSize, char * pDst, size_t dstSize);
