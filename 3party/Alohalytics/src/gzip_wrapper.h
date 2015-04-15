/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/
#include <cstring>  // std::memset
#include <string>
#include <vector>
#include <zlib.h>

#include "logger.h"

namespace alohalytics {

static constexpr size_t kGzipBufferSize = 32768;

struct GzipErrorException : public std::exception {
  std::string msg_;
  GzipErrorException(int err, const char* msg) {
    msg_ = std::string("ERROR ") + std::to_string(err) + " while gzipping with zlib. " + (msg ? msg : "");
  }
  virtual char const* what() const noexcept { return msg_.c_str(); }
};

// Throws GzipErrorException on any gzip processing error.
inline std::string Gzip(const std::string& data_to_compress) {
  z_stream z;
  std::memset(&z, 0, sizeof(z));
  int res = ::deflateInit2(&z, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
  if (Z_OK == res) {
    z.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data_to_compress.data()));
    // TODO(AlexZ): Check situation when uInt is < than size of the data to compress.
    z.avail_in = static_cast<uInt>(data_to_compress.size());
    std::string compressed;
    std::vector<Bytef> buffer;
    buffer.resize(std::min(data_to_compress.size(), kGzipBufferSize));
    do {
      z.next_out = buffer.data();
      z.avail_out = static_cast<uInt>(buffer.size());
      res = ::deflate(&z, Z_FINISH);
      if (compressed.size() < z.total_out) {
        compressed.append(reinterpret_cast<const char*>(buffer.data()), z.total_out - compressed.size());
      }
    } while (Z_OK == res);
    ::deflateEnd(&z);
    if (Z_STREAM_END == res) {
      return compressed;
    }
  }
  throw GzipErrorException(res, z.msg);
}

struct GunzipErrorException : public std::exception {
  std::string msg_;
  GunzipErrorException(int err, const char* msg) {
    msg_ = std::string("ERROR ") + std::to_string(err) + " while gunzipping with zlib. " + (msg ? msg : "");
  }
  virtual char const* what() const noexcept { return msg_.c_str(); }
};

// Throws GunzipErrorException on any gunzip processing error.
inline std::string Gunzip(const std::string& data_to_decompress) {
  z_stream z;
  std::memset(&z, 0, sizeof(z));
  int res = ::inflateInit2(&z, 16 + MAX_WBITS);
  if (Z_OK == res) {
    z.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data_to_decompress.data()));
    z.avail_in = static_cast<uInt>(data_to_decompress.size());
    std::string decompressed;
    std::vector<Bytef> buffer;
    buffer.resize(std::min(data_to_decompress.size(), kGzipBufferSize));
    do {
      z.next_out = buffer.data();
      z.avail_out = static_cast<uInt>(buffer.size());
      res = ::inflate(&z, Z_NO_FLUSH);
      if (decompressed.size() < z.total_out) {
        decompressed.append(reinterpret_cast<char const*>(buffer.data()), z.total_out - decompressed.size());
      }
    } while (Z_OK == res);
    ::inflateEnd(&z);
    if (Z_STREAM_END == res) {
      return decompressed;
    }
  }
  throw GunzipErrorException(res, z.msg);
}

}  // namespace alohalytics
