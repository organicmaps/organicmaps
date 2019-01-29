#ifndef COURGETTE_THIRD_PARTY_BSDIFF_BSDIFF_HEADER_H_
#define COURGETTE_THIRD_PARTY_BSDIFF_BSDIFF_HEADER_H_

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include <string>

namespace bsdiff {
// The following declarations are common to the patch-creation and
// patch-application code.

int constexpr kNumStreams = 6;

enum BSDiffStatus {
  OK = 0,
  MEM_ERROR = 1,
  CRC_ERROR = 2,
  READ_ERROR = 3,
  UNEXPECTED_ERROR = 4,
  WRITE_ERROR = 5,
  CANCELLED = 6,
};

// The patch stream starts with a MBSPatchHeader.
typedef struct MBSPatchHeader_ {
  char tag[8];      // Contains MBS_PATCH_HEADER_TAG.
  uint32_t slen;    // Length of the file to be patched.
  uint32_t scrc32;  // CRC32 of the file to be patched.
  uint32_t dlen;    // Length of the result file.
} MBSPatchHeader;

// This is the value for the tag field.  Must match length exactly, not counting
// null at end of string.
#define MBS_PATCH_HEADER_TAG "GBSDIF42"

template <typename Sink>
void WriteHeader(Sink & sink, MBSPatchHeader* header) {
  sink.Write(header->tag, sizeof(header->tag));
  WriteVarUint(sink, header->slen);
  WriteVarUint(sink, header->scrc32);
  WriteVarUint(sink, header->dlen);
}

template <typename Source>
BSDiffStatus MBS_ReadHeader(Source & src, MBSPatchHeader* header) {
  src.Read(header->tag, sizeof(header->tag));
  header->slen = ReadVarUint<uint32_t>(src);
  header->scrc32 = ReadVarUint<uint32_t>(src);
  header->dlen = ReadVarUint<uint32_t>(src);

  // The string will have a NUL terminator that we don't use, hence '-1'.
  static_assert(sizeof(MBS_PATCH_HEADER_TAG) - 1 == sizeof(header->tag),
                "MBS_PATCH_HEADER_TAG must match header field size");
  if (memcmp(header->tag, MBS_PATCH_HEADER_TAG, 8) != 0)
    return UNEXPECTED_ERROR;

  return OK;
}

inline std::string DebugPrint(BSDiffStatus status) {
  switch (status) {
    case OK: return "OK";
    case MEM_ERROR: return "MEM_ERROR";
    case CRC_ERROR: return "CRC_ERROR";
    case READ_ERROR: return "READ_ERROR";
    case UNEXPECTED_ERROR: return "UNEXPECTED_ERROR";
    case WRITE_ERROR: return "WRITE_ERROR";
    case CANCELLED: return "CANCELLED";
  }
  return "Unknown status";
}
}  // namespace bsdiff

#endif  // COURGETTE_THIRD_PARTY_BSDIFF_BSDIFF_HEADER_H_
