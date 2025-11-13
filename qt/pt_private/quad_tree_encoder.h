#pragma once

#include <cstdint>   // For std::uint64_t, std::int64_t
#include <algorithm> // For std::clamp
#include <bit>       // For std::rotl, std::rotr (C++20)
#include <format>

/**
 * @brief A utility namespace for converting between Latitude/Longitude and a
 * 64-bit quad-tree encoded integer.
 *
 * A 64-bit quad-tree key interleaves 32 bits for latitude and 32 bits for
 * longitude.
 *
 * Precision: The 32 bits per coordinate (180° range for lat, 360° for lon)
 * result in sub-millimeter precision.
 */
namespace QuadTreeEncoder {

    /**
     * @brief A simple struct to store Latitude and Longitude.
     * Values are clamped to their valid ranges upon construction.
     */
    struct LatLon {
        double latitude;
        double longitude;

        /**
         * @brief Constructs a LatLon object, clamping values to valid ranges.
         * @param lat Latitude (-90 to 90).
         * @param lon Longitude (-180 to 180).
         */
        LatLon(double lat, double lon)
            : latitude(std::clamp(lat, -90.0, 90.0)),
              longitude(std::clamp(lon, -180.0, 180.0)) {}
    };

    // --- Constants ---

    // Number of bits used for one coordinate
    static constexpr int BITS_PER_COORD = 32;

    // Total number of levels in the quad-tree
    static constexpr int TOTAL_LEVELS = 32;

    // Full range of latitude
    static constexpr double LAT_RANGE = 180.0; // -90 to +90
    // Full range of longitude
    static constexpr double LON_RANGE = 360.0; // -180 to +180

    // The scale factor for 32 bits (2^32)
    static constexpr double COORD_SCALE = static_cast<double>(1ULL << BITS_PER_COORD);

    // The maximum integer value for a 32-bit coordinate (2^32 - 1)
    static constexpr uint64_t MAX_COORD_VAL = (1ULL << BITS_PER_COORD) - 1;

    // Obfuscation Constants (MUST be kept secret)
    static constexpr uint64_t SECRET_SALT = 0x7A8919B2D4F0C735ULL;
    static constexpr int ROTATE_SHIFT = 19; // A prime number for better diffusion
    static constexpr std::string_view BASE_62_ALPHABET="cAdHgYun0WZNF825ImRqPVklveyKS9pJXLtDQOf1soGaC6zjr4xwUTb7BME3ih";

    /**
     * @brief Encodes a latitude and longitude into a 64-bit quad-tree key.
     *
     * @param latitude The latitude, in degrees (-90 to 90).
     * @param longitude The longitude, in degrees (-180 to 180).
     * @return A 64-bit unsigned long representing the interleaved quad-tree key
     * [Lat31, Lon31, Lat30, Lon30, ... Lat0, Lon0].
     */
    inline uint64_t encode(double latitude, double longitude) {
        // 1. Normalize coordinates to [0.0, 1.0]
        double normLat = (latitude + 90.0) / LAT_RANGE;   // [0.0, 1.0]
        double normLon = (longitude + 180.0) / LON_RANGE; // [0.0, 1.0]

        // 2. Scale to 32-bit integer range [0, 2^32 - 1]
        // Clamp edge cases (90.0, 180.0) to the max value.
        auto latBits = std::min(static_cast<uint64_t>(normLat * COORD_SCALE), MAX_COORD_VAL);
        auto lonBits = std::min(static_cast<uint64_t>(normLon * COORD_SCALE), MAX_COORD_VAL);

        // 3. Interleave the bits
        uint64_t quadkey = 0ULL;
        for (int i = BITS_PER_COORD - 1; i >= 0; --i) {
            uint64_t latBit = (latBits >> i) & 1;
            uint64_t lonBit = (lonBits >> i) & 1;
            quadkey = (quadkey << 1) | latBit;
            quadkey = (quadkey << 1) | lonBit;
        }

        return quadkey;
    }

    /**
     * @brief Obfuscates the raw quad-tree key using a reversible
     * XOR and bit rotation.
     *
     * @param quadkey The raw 64-bit quad-tree key.
     * @return The obfuscated 64-bit key.
     */
    inline uint64_t obfuscate(uint64_t quadkey) {
        // Step 1: Simple diffusion via left rotation
        uint64_t obfuscatedKey = std::rotl(quadkey, ROTATE_SHIFT);
        // Step 2: XOR with a secret salt
        obfuscatedKey ^= SECRET_SALT;
        return obfuscatedKey;
    }

    /**
     * @brief Converts a 64-bit unsigned integer to a base-62 encoded string.
     *
     * @param n The 64-bit unsigned integer to convert.
     * @return A base-62 encoded string representation of the integer, max 11 characters.
     */
    std::string to_base62_string(uint64_t n) {
    if (n == 0)
        return "0";

    const uint64_t base = BASE_62_ALPHABET.size();

    std::string result;
    result.reserve(11); // Max length for 64-bit in base-62
    while (n > 0) {
        uint64_t const remainder = n % base;
        result += BASE_62_ALPHABET[remainder];
        n /= base;
    }

    // The digits were added in reverse order (least significant first), so reverse the string
    std::reverse(result.begin(), result.end());

    return result;
}

    /**
     * @brief Converts a base-62 encoded string back to a 64-bit unsigned integer.
     *
     * @param str The base-62 encoded string.
     * @return The decoded 64-bit unsigned integer.
     * @throws std::invalid_argument if the string contains invalid characters.
     */
uint64_t from_base62_string(const std::string& str) {
    uint64_t const base = BASE_62_ALPHABET.size();
    uint64_t result = 0;
    for (char c : str) {
        size_t const index = BASE_62_ALPHABET.find(c);
        if (index == std::string::npos) {
            throw std::invalid_argument(std::format("Invalid character {} in base-{} string", c, base));
        }
        result = result * base + index;
    }
    return result;
}


    /**
     * @brief De-obfuscates the scrambled 64-bit key back into the raw key.
     * This is the exact inverse of the obfuscate method.
     *
     * @param obfuscatedKey The scrambled 64-bit key.
     * @return The raw 64-bit quad-tree key.
     */
    inline uint64_t deObfuscate(uint64_t obfuscatedKey) {
        // Step 1: XOR back with the secret salt
        uint64_t rawKey = obfuscatedKey ^ SECRET_SALT;
        // Step 2: Reverse the left rotation with a right rotation
        rawKey = std::rotr(rawKey, ROTATE_SHIFT);
        return rawKey;
    }

    /**
     * @brief Decodes a 64-bit quad-tree long back into a latitude and longitude.
     *
     * @param quadkey The 64-bit quad-tree key.
     * @return A @see LatLon object representing the center of the decoded
     * quad-tree cell.
     */
    inline LatLon decode(uint64_t quadkey) {
        uint64_t latBits = 0ULL;
        uint64_t lonBits = 0ULL;

        // 2. De-interleave the bits
        for (int i = (TOTAL_LEVELS * 2) - 2; i >= 0; i -= 2) {
            uint64_t latBit = (quadkey >> (i + 1)) & 1;
            uint64_t lonBit = (quadkey >> i) & 1;
            latBits = (latBits << 1) | latBit;
            lonBits = (lonBits << 1) | lonBit;
        }

        // 3. Un-scale back to normalized [0.0, 1.0]
        // We add 0.5 to get the *center* of the cell.
        double normLat = (static_cast<double>(latBits) + 0.5) / COORD_SCALE;
        double normLon = (static_cast<double>(lonBits) + 0.5) / COORD_SCALE;

        // 4. Un-normalize back to degrees
        double lat = normLat * LAT_RANGE - 90.0;
        double lon = normLon * LON_RANGE - 180.0;

        return LatLon(lat, lon);
    }

    inline std::string LatLonToBase62(double latitude, double longitude) {
        uint64_t const rawKey = encode(latitude, longitude);
        uint64_t const obfuscatedKey = obfuscate(rawKey);
        return to_base62_string(obfuscatedKey);
    }

    inline LatLon Base62ToLatLon(std::string const & base62) {
        auto const obfuscatedKey = from_base62_string(base62);
        uint64_t const rawKey = deObfuscate(obfuscatedKey);
        return decode(rawKey);
    }

} // namespace QuadTreeEncoder
