#pragma once

#include "3party/BLAKE3/c/blake3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace coding
{
// BLAKE3 hash, used for fast integrity verification of map files.
// Provides incremental (streaming) hashing as well as one-shot helpers.
//
// BLAKE3 is an extendable-output function: any prefix of the 32-byte digest is
// itself a valid shorter hash. The *Base64 helpers can therefore emit a
// truncated digest (e.g. 9 bytes) for compact storage in countries.json.
class Blake3
{
public:
  static size_t constexpr kHashSizeInBytes = BLAKE3_OUT_LEN;  // 32
  using Hash = std::array<uint8_t, kHashSizeInBytes>;

  Blake3();

  // Feeds the next portion of data into the hash. Can be called repeatedly.
  void Update(void const * data, size_t size);

  // Completes hashing and returns the full digest. Does not modify the state,
  // so Update() may still be called afterwards to hash more data.
  Hash Finalize() const;

  // Same as Finalize(), but returns base64 of the first |numBytes| of the digest.
  std::string FinalizeToBase64(size_t numBytes = kHashSizeInBytes) const;

  // One-shot helpers.
  static Hash Calculate(std::string const & filePath);
  static std::string CalculateBase64(std::string const & filePath, size_t numBytes = kHashSizeInBytes);
  // Same as CalculateBase64, but truncated to the per-map integrity hash size used
  // for map files referenced in countries.json. See the definition for details.
  static std::string CalculateMwmBase64(std::string const & filePath);
  static Hash CalculateForString(std::string_view str);

private:
  blake3_hasher m_hasher;
};
}  // namespace coding
