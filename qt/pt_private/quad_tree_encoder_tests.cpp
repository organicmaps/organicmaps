#include "quad_tree_encoder.h"

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept> // For std::runtime_error
#include <cmath>     // For std::abs
#include <format>    // For std::format (C++20)
#include <limits>    // For std::numeric_limits
#include <iomanip>   // For std::hex
#include <string>
#include <algorithm> // For std::reverse
#include <cstdint>   // For uint64_t


// The precision for latitude is half the size of one 32-bit bucket.
// (180 degrees / 2^32) / 2
static constexpr double LAT_DELTA = (180.0 / (1ULL << 32)) / 2.0 + 1e-7;

// The precision for longitude is half the size of one 32-bit bucket.
// (360 degrees / 2^32) / 2
static constexpr double LON_DELTA = (360.0 / (1ULL << 32)) / 2.0 + 1e-7;


// --- Custom Assertion Helpers ---

/**
 * Asserts that two doubles are equal within a given delta.
 * Throws a std::runtime_error if the assertion fails.
 */
void assertEquals(double expected, double actual, double delta, const std::string& message) {
    if (std::abs(expected - actual) > delta) {
        throw std::runtime_error(std::format(
            "{}. Expected: {:.15f}, Actual: {:.15f} (Delta: {:.15f})",
            message, expected, actual, delta
        ));
    }
}

/**
 * Asserts that two 64-bit unsigned integers are equal.
 * Throws a std::runtime_error if the assertion fails.
 */
void assertEquals(std::uint64_t expected, std::uint64_t actual, const std::string& message) {
    if (expected != actual) {
        throw std::runtime_error(std::format(
            "{}. Expected: 0x{:X}, Actual: 0x{:X}",
            message, expected, actual
        ));
    }
}

/**
 * Asserts that a condition is true.
 * Throws a std::runtime_error if the assertion fails.
 */
void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// --- Test Cases ---

void testLatLonRecord() {
    // Test clamping
    QuadTreeEncoder::LatLon clamped(100.0, -200.0);
    assertEquals(90.0, clamped.latitude, 1e-9, "Latitude clamping failed");
    assertEquals(-180.0, clamped.longitude, 1e-9, "Longitude clamping failed");

    // Test normal values
    QuadTreeEncoder::LatLon normal(45.0, 120.0);
    assertEquals(45.0, normal.latitude, 1e-9, "Latitude normal value failed");
    assertEquals(120.0, normal.longitude, 1e-9, "Longitude normal value failed");
}

void testEncodeOrigin() {
    // lat=0, lon=0 -> normLat=0.5, normLon=0.5
    std::uint64_t expectedKey = 0xC000000000000000ULL;
    auto actualKey = QuadTreeEncoder::encode(0.0, 0.0);
    assertEquals(expectedKey, actualKey, "Encoding (0, 0) failed");
}

void testEncodeMinCorner() {
    // lat=-90, lon=-180 -> normLat=0.0, normLon=0.0
    std::uint64_t expectedKey = 0x0000000000000000ULL;
    auto actualKey = QuadTreeEncoder::encode(-90.0, -180.0);
    assertEquals(expectedKey, actualKey, "Encoding (-90, -180) failed");
}

void testEncodeMaxCorner() {
    // lat=90, lon=180 -> normLat=1.0, normLon=1.0
    std::uint64_t expectedKey = 0xFFFFFFFFFFFFFFFFULL; // std::numeric_limits<std::uint64_t>::max();
    auto actualKey = QuadTreeEncoder::encode(90.0, 180.0);
    assertEquals(expectedKey, actualKey, "Encoding (90, 180) failed");
}

void testDecodeOrigin() {
    std::uint64_t key = 0xC000000000000000ULL; // 1100...00
    auto decoded = QuadTreeEncoder::decode(key);
    // The decoded value should be the center of the (0,0) bucket.
    assertEquals(0.0, decoded.latitude, LAT_DELTA * 2.0, "Decoding origin latitude failed");
    assertEquals(0.0, decoded.longitude, LON_DELTA * 2.0, "Decoding origin longitude failed");
}

void testDecodeMinCorner() {
    std::uint64_t key = 0x0000000000000000ULL; // 0000...00
    auto decoded = QuadTreeEncoder::decode(key);
    // Should be the center of the first bucket, close to -90, -180
    assertEquals(-90.0, decoded.latitude, LAT_DELTA * 2.0, "Decoding min corner latitude failed");
    assertEquals(-180.0, decoded.longitude, LON_DELTA * 2.0, "Decoding min corner longitude failed");
}

void testDecodeMaxCorner() {
    std::uint64_t key = 0xFFFFFFFFFFFFFFFFULL; // 1111...11
    auto decoded = QuadTreeEncoder::decode(key);
    // Should be the center of the last bucket, close to 90, 180
    assertEquals(90.0, decoded.latitude, LAT_DELTA * 2.0, "Decoding max corner latitude failed");
    assertEquals(180.0, decoded.longitude, LON_DELTA * 2.0, "Decoding max corner longitude failed");
}

// Using a simple struct to hold parameterized test data
// (C++20 designated initializers used below)
struct TestLocation {
    double lat;
    double lon;
    std::string name;
};

void testObfuscationReversibility() {
    std::cout << "  Running obfuscation reversibility sub-tests..." << std::endl;
    std::uint64_t testKeys[] = {
        0x0000000000000000ULL,
        0xFFFFFFFFFFFFFFFFULL,
        0x123456789ABCDEF0ULL,
        0x0FEDCBA987654321ULL,
        0xC000000000000000ULL,
        0xAAAAAAAAAAAAAAAAULL,
    };

    for (std::uint64_t originalKey : testKeys) {
        auto obfuscated = QuadTreeEncoder::obfuscate(originalKey);
        auto deObfuscated = QuadTreeEncoder::deObfuscate(obfuscated);

        assertEquals(originalKey, deObfuscated,
            std::format("Obfuscation/De-obfuscation failed for key 0x{:X}", originalKey));
    }
    std::cout << "  ...all obfuscation reversibility sub-tests passed." << std::endl;
}

void testEndToEndRoundtrip() {
    std::cout << "  Running END-TO-END roundtrip sub-tests (Encode -> Obfuscate -> De-Obfuscate -> Decode)..." << std::endl;
    const std::vector<TestLocation> locations = {
        {.lat = 47.3769,    .lon = 8.5417,     .name = "Zurich"},
        {.lat = -33.8688,   .lon = 151.2093,   .name = "Sydney"},
        {.lat = 0.0,        .lon = 0.0,        .name = "Origin"},
        {.lat = 89.999999,  .lon = 179.999999, .name = "Near Max Corner"},
        {.lat = 35.6895,    .lon = 139.6917,   .name = "Tokyo"},
        {.lat = 40.7128,    .lon = -74.0060,   .name = "New York"},
        {.lat = -90.0,      .lon = -180.0,     .name = "Min Corner"},
        {.lat = 29.9792,    .lon = 31.1342,    .name = "Giza"}
    };

    for (const auto& loc : locations) {
        // 1. Encode
        auto rawKey = QuadTreeEncoder::encode(loc.lat, loc.lon);
        // 2. Obfuscate
        auto obfuscatedKey = QuadTreeEncoder::obfuscate(rawKey);
        // 3. De-obfuscate
        auto deObfuscatedKey = QuadTreeEncoder::deObfuscate(obfuscatedKey);

        std::cout << std::format("  Raw Key=0x{:X}, Obf Key=0x{:X}, DeObf Key=0x{:X}\n",
            rawKey, obfuscatedKey, deObfuscatedKey);

        // Assertion 1: Keys must match
        assertEquals(rawKey, deObfuscatedKey,
            std::format("End-to-end failed at key stage for {}", loc.name));

        // 4. Decode
        auto decoded = QuadTreeEncoder::decode(deObfuscatedKey);

        std::string messageBase = std::format(
            "End-to-end (full cycle) failed for {} ({:.8f}, {:.8f}). Decoded to ({:.8f}, {:.8f})",
            loc.name, loc.lat, loc.lon, decoded.latitude, decoded.longitude
        );

        // Assertion 2: Coordinates must match within precision
        assertTrue(std::abs(loc.lat - decoded.latitude) <= LAT_DELTA,
            messageBase + " - Latitude out of bounds.");
        assertTrue(std::abs(loc.lon - decoded.longitude) <= LON_DELTA,
            messageBase + " - Longitude out of bounds.");
    }
    std::cout << "  ...all END-TO-END roundtrip sub-tests passed." << std::endl;
}

void testRoundtrip() {
    const std::vector<TestLocation> locations = {
        {.lat = 47.3769,    .lon = 8.5417,     .name = "Zurich"},
        {.lat = -33.8688,   .lon = 151.2093,   .name = "Sydney"},
        {.lat = 35.6895,    .lon = 139.6917,   .name = "Tokyo"},
        {.lat = 40.7128,    .lon = -74.0060,   .name = "New York"},
        {.lat = 0.0,        .lon = 0.0,        .name = "Origin"},
        {.lat = -90.0,      .lon = -180.0,     .name = "Min Corner"},
        {.lat = 89.999999,  .lon = 179.999999, .name = "Near Max Corner"},
        {.lat = 29.9792,    .lon = 31.1342,    .name = "Giza"}
    };

    std::cout << "  Running original roundtrip sub-tests..." << std::endl;
    for (const auto& loc : locations) {
        auto key = QuadTreeEncoder::encode(loc.lat, loc.lon);
        auto str = QuadTreeEncoder::to_base62_string(key);
        auto decodedKey = QuadTreeEncoder::from_base62_string(str);
        // assertEquals(key, decodedKey,
        //     std::format("Roundtrip failed at key stage for {}", loc.name));
        auto decoded = QuadTreeEncoder::decode(decodedKey);

        std::string messageBase = std::format(
            "Roundtrip failed for {} ({:.8f}, {:.8f}). Decoded to ({:.8f}, {:.8f})",
            loc.name, loc.lat, loc.lon, decoded.latitude, decoded.longitude
        );

        assertTrue(std::abs(loc.lat - decoded.latitude) <= LAT_DELTA,
            messageBase + " - Latitude out of bounds.");
        assertTrue(std::abs(loc.lon - decoded.longitude) <= LON_DELTA,
            messageBase + " - Longitude out of bounds.");
    }
    std::cout << "  ...all original roundtrip sub-tests passed." << std::endl;
}

// --- Test Runner ---

/**
 * A simple test runner for a single test case.
 * @param testMethod The test method to run.
 * @param testName The name of the test for logging.
 * @return true if the test passed, false if it threw an exception.
 */
bool runTest(const std::function<void()>& testMethod, const std::string& testName) {
    try {
        testMethod();
        std::cout << "PASS: " << testName << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << testName << std::endl;
        // Print the error message for the failed assertion
        std::cerr << "      " << e.what() << std::endl;
        return false;
    }
}

int main() {
    std::cout << "Running QuadTreeEncoder 64-bit tests (C++23)..." << std::endl;
    int passed = 0;
    int failed = 0;

    // A simple macro to reduce boilerplate
    #define RUN_TEST(testFunc) \
        if (runTest(testFunc, #testFunc)) passed++; else failed++;

    // Run all test methods and count passes/failures
    RUN_TEST(testLatLonRecord);
    RUN_TEST(testEncodeOrigin);
    RUN_TEST(testEncodeMinCorner);
    RUN_TEST(testEncodeMaxCorner);
    RUN_TEST(testDecodeOrigin);
    RUN_TEST(testDecodeMinCorner);
    RUN_TEST(testDecodeMaxCorner);
    RUN_TEST(testObfuscationReversibility);
    RUN_TEST(testEndToEndRoundtrip);
    RUN_TEST(testRoundtrip);

    #undef RUN_TEST // Clean up the macro

    // Print summary
    std::cout << "\n--- Test Summary ---" << std::endl;
    std::cout << std::format("Total: {}, Passed: {}, Failed: {}\n", (passed + failed), passed, failed);

    // Exit with a non-zero status code if any tests failed
    if (failed > 0) {
        std::cerr << "\n!!! SOME TESTS FAILED !!!" << std::endl;
        return 1;
    } else {
        std::cout << "\nAll tests passed successfully." << std::endl;
        return 0;
    }
}
