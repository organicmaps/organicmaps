// Copyright 2003, 2004 Colin Percival
// All rights reserved
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted providing that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// For the terms under which this work may be distributed, please see
// the adjoining file "LICENSE".
//
// Changelog:
// 2005-04-26 - Define the header as a C structure, add a CRC32 checksum to
//              the header, and make all the types 32-bit.
//                --Benjamin Smedberg <benjamin@smedbergs.us>
// 2009-03-31 - Change to use Streams.  Move CRC code to crc.{h,cc}
//              Changed status to an enum, removed unused status codes.
//                --Stephen Adams <sra@chromium.org>
// 2013-04-10 - Added wrapper to apply a patch directly to files.
//                --Joshua Pawlicki <waffles@chromium.org>
// 2017-08-14 - Moved "apply" and "create" to the header file, rewrote
//              all routines to use MAPS.ME readers and writers instead
//              of Courgette streams and files.
//                --Maxim Pimenov <m@maps.me>
// 2019-01-24 - Got rid of the paged array. We have enough address space
//              for our application of bsdiff.
//                --Maxim Pimenov <m@maps.me>

// Changelog for bsdiff_apply:
// 2009-03-31 - Change to use Streams.  Move CRC code to crc.{h,cc}
//                --Stephen Adams <sra@chromium.org>
// 2013-04-10 - Add wrapper method to apply a patch to files directly.
//                --Joshua Pawlicki <waffles@chromium.org>

// Changelog for bsdiff_create:
// 2005-05-05 - Use the modified header struct from bspatch.h; use 32-bit
//              values throughout.
//                --Benjamin Smedberg <benjamin@smedbergs.us>
// 2005-05-18 - Use the same CRC algorithm as bzip2, and leverage the CRC table
//              provided by libbz2.
//                --Darin Fisher <darin@meer.net>
// 2007-11-14 - Changed to use Crc from Lzma library instead of Bzip library
//                --Rahul Kuchhal
// 2009-03-31 - Change to use Streams.  Added lots of comments.
//                --Stephen Adams <sra@chromium.org>
// 2010-05-26 - Use a paged array for V and I. The address space may be too
//              fragmented for these big arrays to be contiguous.
//                --Stephen Adams <sra@chromium.org>
// 2015-08-03 - Extract qsufsort portion to a separate file.
//                --Samuel Huang <huangs@chromium.org>
// 2015-08-12 - Interface change to search().
//                --Samuel Huang <huangs@chromium.org>
// 2016-07-29 - Replacing qsufsort with divsufsort.
//                --Samuel Huang <huangs@chromium.org>

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COURGETTE_THIRD_PARTY_BSDIFF_BSDIFF_H_
#define COURGETTE_THIRD_PARTY_BSDIFF_BSDIFF_H_

#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/cancellable.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include <array>
#include <cstdint>
#include <sstream>
#include <vector>

#include "3party/bsdiff-courgette/bsdiff/bsdiff_common.h"
#include "3party/bsdiff-courgette/bsdiff/bsdiff_search.h"
#include "3party/bsdiff-courgette/divsufsort/divsufsort.h"

#include "zlib.h"

namespace bsdiff {
// A MemWriter with its own buffer.
struct MemStream {
  MemStream(): m_writer(m_buf) {}

  MemWriter<std::vector<uint8_t>> & GetWriter() { return m_writer; }
  size_t const Size() const { return m_buf.size(); }
  std::vector<uint8_t> const & GetBuf() const { return m_buf; }

private:
  std::vector<uint8_t> m_buf;
  MemWriter<std::vector<uint8_t>> m_writer;
};

inline uint32_t CalculateCrc(const uint8_t* buffer, size_t size) {
  // Calculate Crc by calling CRC method in zlib
  const auto size32 = base::checked_cast<uint32_t>(size);
  const uint32_t crc = base::checked_cast<uint32_t>(crc32(0, buffer, size32));
  return ~crc;
}

// Creates a binary patch.
template <typename OldReader, typename NewReader, typename PatchSink>
BSDiffStatus CreateBinaryPatch(OldReader & old_reader,
                               NewReader & new_reader,
                               PatchSink & patch_sink) {
  ReaderSource<OldReader> old_source(old_reader);
  ReaderSource<NewReader> new_source(new_reader);

  auto initial_patch_sink_pos = patch_sink.Pos();

  base::Timer bsdiff_timer;

  CHECK_GREATER_OR_EQUAL(kNumStreams, 6, ());
  std::array<MemStream, kNumStreams> mem_streams;
  auto & control_stream_copy_counts = mem_streams[0];
  auto & control_stream_extra_counts = mem_streams[1];
  auto & control_stream_seeks = mem_streams[2];
  auto & diff_skips = mem_streams[3];
  auto & diff_bytes = mem_streams[4];
  auto & extra_bytes = mem_streams[5];

  const int old_size = static_cast<int>(old_source.Size());
  std::vector<uint8_t> old_buf(old_size);
  old_source.Read(old_buf.data(), old_buf.size());
  const uint8_t * old = old_buf.data();

  std::vector<divsuf::saidx_t> suffix_array(old_size + 1);
  base::Timer suf_sort_timer;
  divsuf::saint_t result = divsuf::divsufsort_include_empty(old, suffix_array.data(), old_size);
  LOG(LINFO, ("Done divsufsort", suf_sort_timer.ElapsedSeconds()));
  if (result != 0)
    return UNEXPECTED_ERROR;

  const int new_size = static_cast<int>(new_source.Size());
  std::vector<uint8_t> new_buf(new_size);
  new_source.Read(new_buf.data(), new_buf.size());
  const uint8_t * newbuf = new_buf.data();

  int control_length = 0;
  int diff_bytes_length = 0;
  int diff_bytes_nonzero = 0;
  int extra_bytes_length = 0;

  // The patch format is a sequence of triples <copy,extra,seek> where 'copy' is
  // the number of bytes to copy from the old file (possibly with mistakes),
  // 'extra' is the number of bytes to copy from a stream of fresh bytes, and
  // 'seek' is an offset to move to the position to copy for the next triple.
  //
  // The invariant at the top of this loop is that we are committed to emitting
  // a triple for the part of |newbuf| surrounding a 'seed' match near
  // |lastscan|.  We are searching for a second match that will be the 'seed' of
  // the next triple.  As we scan through |newbuf|, one of four things can
  // happen at the current position |scan|:
  //
  //  1. We find a nice match that appears to be consistent with the current
  //     seed.  Continue scanning.  It is likely that this match will become
  //     part of the 'copy'.
  //
  //  2. We find match which does much better than extending the current seed
  //     old match.  Emit a triple for the current seed and take this match as
  //     the new seed for a new triple.  By 'much better' we remove 8 mismatched
  //     bytes by taking the new seed.
  //
  //  3. There is not a good match.  Continue scanning.  These bytes will likely
  //     become part of the 'extra'.
  //
  //  4. There is no match because we reached the end of the input, |newbuf|.

  // This is how the loop advances through the bytes of |newbuf|:
  //
  // ...012345678901234567890123456789...
  //    ssssssssss                      Seed at |lastscan|
  //              xxyyyxxyyxy           |scan| forward, cases (3)(x) & (1)(y)
  //                         mmmmmmmm   New match will start new seed case (2).
  //    fffffffffffffff                 |lenf| = scan forward from |lastscan|
  //                     bbbb           |lenb| = scan back from new seed |scan|.
  //    ddddddddddddddd                 Emit diff bytes for the 'copy'.
  //                   xx               Emit extra bytes.
  //                     ssssssssssss   |lastscan = scan - lenb| is new seed.
  //                                 x  Cases (1) and (3) ....

  int lastscan = 0, lastpos = 0, lastoffset = 0;
  int scan = 0;
  SearchResult match(0, 0);
  uint32_t pending_diff_zeros = 0;

  while (scan < new_size) {
    int oldscore = 0;  // Count of how many bytes of the current match at |scan|
                       // extend the match at |lastscan|.
    match.pos = 0;

    scan += match.size;
    for (int scsc = scan; scan < new_size; ++scan) {
      match = search<decltype(suffix_array)>(suffix_array, old, old_size, newbuf + scan,
                                             new_size - scan);

      for (; scsc < scan + match.size; scsc++)
        if ((scsc + lastoffset < old_size) &&
            (old[scsc + lastoffset] == newbuf[scsc]))
          oldscore++;

      if ((match.size == oldscore) && (match.size != 0))
        break;  // Good continuing match, case (1)
      if (match.size > oldscore + 8)
        break;  // New seed match, case (2)

      if ((scan + lastoffset < old_size) &&
          (old[scan + lastoffset] == newbuf[scan]))
        oldscore--;
      // Case (3) continues in this loop until we fall out of the loop (4).
    }

    if ((match.size != oldscore) || (scan == new_size)) {  // Cases (2) and (4)
      // This next chunk of code finds the boundary between the bytes to be
      // copied as part of the current triple, and the bytes to be copied as
      // part of the next triple.  The |lastscan| match is extended forwards as
      // far as possible provided doing to does not add too many mistakes.  The
      // |scan| match is extended backwards in a similar way.

      // Extend the current match (if any) backwards.  |lenb| is the maximal
      // extension for which less than half the byte positions in the extension
      // are wrong.
      int lenb = 0;
      if (scan < new_size) {  // i.e. not case (4); there is a match to extend.
        int score = 0, Sb = 0;
        for (int i = 1; (scan >= lastscan + i) && (match.pos >= i); i++) {
          if (old[match.pos - i] == newbuf[scan - i])
            score++;
          if (score * 2 - i > Sb * 2 - lenb) {
            Sb = score;
            lenb = i;
          }
        }
      }

      // Extend the lastscan match forward; |lenf| is the maximal extension for
      // which less than half of the byte positions in entire lastscan match are
      // wrong.  There is a subtle point here: |lastscan| points to before the
      // seed match by |lenb| bytes from the previous iteration.  This is why
      // the loop measures the total number of mistakes in the the match, not
      // just the from the match.
      int lenf = 0;
      {
        int score = 0, Sf = 0;
        for (int i = 0; (lastscan + i < scan) && (lastpos + i < old_size);) {
          if (old[lastpos + i] == newbuf[lastscan + i])
            score++;
          i++;
          if (score * 2 - i > Sf * 2 - lenf) {
            Sf = score;
            lenf = i;
          }
        }
      }

      // If the extended scans overlap, pick a position in the overlap region
      // that maximizes the exact matching bytes.
      if (lastscan + lenf > scan - lenb) {
        int overlap = (lastscan + lenf) - (scan - lenb);
        int score = 0;
        int Ss = 0, lens = 0;
        for (int i = 0; i < overlap; i++) {
          if (newbuf[lastscan + lenf - overlap + i] ==
              old[lastpos + lenf - overlap + i]) {
            score++;
          }
          if (newbuf[scan - lenb + i] == old[match.pos - lenb + i]) {
            score--;
          }
          if (score > Ss) {
            Ss = score;
            lens = i + 1;
          }
        }

        lenf += lens - overlap;
        lenb -= lens;
      };

      for (int i = 0; i < lenf; i++) {
        uint8_t diff_byte = newbuf[lastscan + i] - old[lastpos + i];
        if (diff_byte) {
          ++diff_bytes_nonzero;
          WriteVarUint(diff_skips.GetWriter(), pending_diff_zeros);
          pending_diff_zeros = 0;
          diff_bytes.GetWriter().Write(&diff_byte, 1);
        } else {
          ++pending_diff_zeros;
        }
      }
      int gap = (scan - lenb) - (lastscan + lenf);
      for (int i = 0; i < gap; i++) {
        extra_bytes.GetWriter().Write(&newbuf[lastscan + lenf + i], 1);
      }

      diff_bytes_length += lenf;
      extra_bytes_length += gap;

      uint32_t copy_count = lenf;
      uint32_t extra_count = gap;
      int32_t seek_adjustment = ((match.pos - lenb) - (lastpos + lenf));

      WriteVarUint(control_stream_copy_counts.GetWriter(), copy_count);
      WriteVarUint(control_stream_extra_counts.GetWriter(), extra_count);
      WriteVarInt(control_stream_seeks.GetWriter(), seek_adjustment);

      ++control_length;

#ifdef DEBUG_bsmedberg
      LOG(LDEBUG, ("Writing a block:  copy:", copy_count, "extra:", extra_count, "seek:", seek_adjustment));
#endif

      lastscan = scan - lenb;  // Include the backward extension in seed.
      lastpos = match.pos - lenb;    //  ditto.
      lastoffset = lastpos - lastscan;
    }
  }

  WriteVarUint(diff_skips.GetWriter(), pending_diff_zeros);

  suffix_array.clear();

  MBSPatchHeader header;
  // The string will have a null terminator that we don't use, hence '-1'.
  static_assert(sizeof(MBS_PATCH_HEADER_TAG) - 1 == sizeof(header.tag),
                "MBS_PATCH_HEADER_TAG must match header field size");
  memcpy(header.tag, MBS_PATCH_HEADER_TAG, sizeof(header.tag));
  header.slen = old_size;
  header.scrc32 = CalculateCrc(old, old_size);
  header.dlen = new_size;

  WriteHeader(patch_sink, &header);
  for (auto const & s : mem_streams)
  {
    uint32_t const sz = base::checked_cast<uint32_t>(s.Size());
    WriteToSink(patch_sink, sz);
  }

  for (auto const & s : mem_streams)
    patch_sink.Write(s.GetBuf().data(), s.GetBuf().size());

  size_t diff_skips_length = diff_skips.Size();

  std::ostringstream log_stream;
  log_stream << "Control tuples: " << control_length
          << "  copy bytes: " << diff_bytes_length
          << "  mistakes: " << diff_bytes_nonzero
          << "  (skips: " << diff_skips_length << ")"
          << "  extra bytes: " << extra_bytes_length
          << "\nUncompressed bsdiff patch size "
          << patch_sink.Pos() - initial_patch_sink_pos
          << "\nEnd bsdiff "
          << bsdiff_timer.ElapsedSeconds();

  LOG(LINFO, (log_stream.str()));

  return OK;
}

// Applies the given patch file to a given source file. This method validates
// the CRC of the original file stored in the patch file, before applying the
// patch to it.
template <typename OldReader, typename NewSink, typename PatchReader>
BSDiffStatus ApplyBinaryPatch(OldReader & old_reader, NewSink & new_sink,
                              PatchReader & patch_reader, const base::Cancellable & cancellable)
{
  ReaderSource<OldReader> old_source(old_reader);
  ReaderSource<PatchReader> patch_source(patch_reader);

  MBSPatchHeader header;
  BSDiffStatus ret = MBS_ReadHeader(patch_source, &header);
  if (ret != OK)
    return ret;

  const auto old_size = static_cast<size_t>(old_source.Size());
  std::vector<uint8_t> old_buf(old_size);
  old_source.Read(old_buf.data(), old_buf.size());

  const uint8_t* old_start = old_buf.data();
  const uint8_t* old_end = old_buf.data() + old_buf.size();
  const uint8_t* old_position = old_start;

  if (old_size != header.slen)
    return UNEXPECTED_ERROR;

  if (CalculateCrc(old_start, old_size) != header.scrc32)
    return CRC_ERROR;

  CHECK_GREATER_OR_EQUAL(kNumStreams, 6, ());
  std::vector<uint32_t> stream_sizes(kNumStreams);
  for (auto & s : stream_sizes)
    s = ReadPrimitiveFromSource<uint32_t>(patch_source);

  std::vector<ReaderSource<PatchReader>> patch_streams;
  patch_streams.reserve(kNumStreams);
  for (size_t i = 0; i < kNumStreams; ++i) {
    uint64_t size = static_cast<uint64_t>(stream_sizes[i]);
    patch_streams.emplace_back(ReaderSource<PatchReader>(patch_source.SubReader(size)));
  }

  auto & control_stream_copy_counts = patch_streams[0];
  auto & control_stream_extra_counts = patch_streams[1];
  auto & control_stream_seeks = patch_streams[2];
  auto & diff_skips = patch_streams[3];
  auto & diff_bytes = patch_streams[4];
  auto & extra_bytes = patch_streams[5];

  std::vector<uint8_t> extra_bytes_buf(static_cast<size_t>(extra_bytes.Size()));
  extra_bytes.Read(extra_bytes_buf.data(), extra_bytes_buf.size());

  const uint8_t* extra_start = extra_bytes_buf.data();
  const uint8_t* extra_end = extra_bytes_buf.data() + extra_bytes_buf.size();
  const uint8_t* extra_position = extra_start;

//  if (header->dlen && !new_sink->Reserve(header->dlen))
//    return MEM_ERROR;

  auto pending_diff_zeros = ReadVarUint<uint32_t>(diff_skips);

  // We will check whether the application process has been cancelled
  // upon copying every |kCheckCancelledPeriod| bytes from the old file.
  constexpr size_t kCheckCancelledPeriod = 100 * 1024;

  while (control_stream_copy_counts.Size() > 0) {
    if (cancellable.IsCancelled())
      return CANCELLED;

    auto copy_count = ReadVarUint<uint32_t>(control_stream_copy_counts);
    auto extra_count = ReadVarUint<uint32_t>(control_stream_extra_counts);
    auto seek_adjustment = ReadVarInt<int32_t>(control_stream_seeks);

#ifdef DEBUG_bsmedberg
    LOG(LDEBUG, ("Applying block:  copy:", copy_count, "extra:", extra_count, "seek:", seek_adjustment));
#endif

    // Byte-wise arithmetically add bytes from old file to bytes from the diff
    // block.
    if (copy_count > static_cast<size_t>(old_end - old_position))
      return UNEXPECTED_ERROR;

    // Add together bytes from the 'old' file and the 'diff' stream.
    for (size_t i = 0; i < copy_count; ++i) {
      if (i > 0 && i % kCheckCancelledPeriod == 0 && cancellable.IsCancelled())
        return CANCELLED;

      uint8_t diff_byte = 0;
      if (pending_diff_zeros) {
        --pending_diff_zeros;
      } else {
        pending_diff_zeros = ReadVarUint<uint32_t>(diff_skips);
        diff_byte = ReadPrimitiveFromSource<uint8_t>(diff_bytes);
      }
      uint8_t byte = old_position[i] + diff_byte;
      WriteToSink(new_sink, byte);
    }
    old_position += copy_count;

    // Copy bytes from the extra block.
    if (extra_count > static_cast<size_t>(extra_end - extra_position))
      return UNEXPECTED_ERROR;

    new_sink.Write(extra_position, extra_count);

    extra_position += extra_count;

    // "seek" forwards (or backwards) in oldfile.
    if (old_position + seek_adjustment < old_start ||
        old_position + seek_adjustment > old_end)
      return UNEXPECTED_ERROR;

    old_position += seek_adjustment;
  }

  if (control_stream_copy_counts.Size() > 0 ||
      control_stream_extra_counts.Size() > 0 ||
      control_stream_seeks.Size() > 0 ||
      diff_skips.Size() > 0 ||
      diff_bytes.Size() > 0 ||
      extra_bytes.Size() > 0)
  {
    return UNEXPECTED_ERROR;
  }

  if (cancellable.IsCancelled())
    return CANCELLED;

  return OK;
}
}  // namespace bsdiff

#endif  // COURGETTE_THIRD_PARTY_BSDIFF_BSDIFF_H_
