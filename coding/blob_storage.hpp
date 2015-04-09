#pragma once

/*
#include "coding/dd_vector.hpp"
#include "coding/polymorph_reader.hpp"
#include "std/function.hpp"
#include "std/unique_ptr.hpp"
#include "std/string.hpp"
#include "base/base.hpp"
#include "base/exception.hpp"

class Reader;

class BlobStorage
{
public:
  DECLARE_EXCEPTION(OpenException, RootException);

  typedef function<void (char const *, size_t, char *, size_t)> DecompressorType;

  // Takes ownership of pReader and deletes it, even if exception is thrown.
  BlobStorage(Reader const * pReader,
              DecompressorType const & decompressor);
  ~BlobStorage();

  // Get blob by its number, starting from 0.
  void GetBlob(uint32_t i, string & blob) const;

  // Returns the number of blobs.
  uint32_t Size() const;

private:
  void Init();

  uint32_t GetChunkFromBI(uint32_t blobInfo) const;
  uint32_t GetOffsetFromBI(uint32_t blobInfo) const;

  uint32_t m_bitsInChunkSize;
  static uint32_t const HEADER_SIZE = 4;

  unique_ptr<Reader const> const m_pReader;
  DecompressorType m_decompressor;

  DDVector<uint32_t, PolymorphReader> m_blobInfo;
  DDVector<uint32_t, PolymorphReader> m_chunkOffset;
};
*/
