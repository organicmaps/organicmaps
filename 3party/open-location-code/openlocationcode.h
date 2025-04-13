// The OpenLocationCode namespace provides a way of encoding between geographic
// coordinates and character strings that use a disambiguated character set.
// The aim is to provide a more convenient way for humans to handle geographic
// coordinates than latitude and longitude pairs.
//
// The codes can be easily read and remembered, and truncating codes converts
// them from a point to an area, meaning that where extreme accuracy is not
// required the codes can be shortened.
#ifndef LOCATION_OPENLOCATIONCODE_OPENLOCATIONCODE_H_
#define LOCATION_OPENLOCATIONCODE_OPENLOCATIONCODE_H_

#include <string>

#include "codearea.h"

namespace openlocationcode {

// Encodes a pair of coordinates and return an Open Location Code representing a
// rectangle that encloses the coordinates. The accuracy of the code is
// controlled by the code length.
//
// Returns an Open Location Code with code_length significant digits. The string
// returned may be one character longer if it includes a separator character
// for formatting.
std::string Encode(const LatLng &location, size_t code_length);

// Encodes a pair of coordinates and return an Open Location Code representing a
// rectangle that encloses the coordinates. The accuracy of the code is
// sufficient to represent a building such as a house, and is approximately
// 13x13 meters at Earth's equator.
std::string Encode(const LatLng &location);

// Decodes an Open Location Code and returns a rectangle that describes the area
// represented by the code.
CodeArea Decode(const std::string &code);

// Removes characters from the start of an OLC code.
// This uses a reference location to determine how many initial characters
// can be removed from the OLC code. The number of characters that can be
// removed depends on the distance between the code center and the reference
// location.
//
// The reference location must be within a safety factor of the maximum range.
// This ensures that the shortened code will be able to be recovered using
// slightly different locations.
//
// If the code isn't a valid full code or is padded, it cannot be shortened and
// the code is returned as-is.
std::string Shorten(const std::string &code, const LatLng &reference_location);

// Recovers the nearest matching code to a specified location.
// Given a short Open Location Code of between four and seven characters,
// this recovers the nearest matching full code to the specified location.
//
// If the code isn't a valid short code, it cannot be recovered and the code
// is returned as-is.
std::string RecoverNearest(const std::string &short_code,
                           const LatLng &reference_location);

// Returns the number of valid Open Location Code characters in a string. This
// excludes invalid characters and separators.
size_t CodeLength(const std::string &code);

// Determines if a code is valid and can be decoded.
// The empty string is a valid code, but whitespace included in a code is not
// valid.
bool IsValid(const std::string &code);

// Determines if a code is a valid short code.
bool IsShort(const std::string &code);

// Determines if a code is a valid full Open Location Code.
//
// Not all possible combinations of Open Location Code characters decode to
// valid latitude and longitude values. This checks that a code is valid
// and also that the latitude and longitude values are legal. If the prefix
// character is present, it must be the first character. If the separator
// character is present, it must be after four characters.
bool IsFull(const std::string &code);

namespace internal {
// The separator character is used to identify strings as OLC codes.
extern const char kSeparator;
// Provides the position of the separator.
extern const size_t kSeparatorPosition;
// Defines the maximum number of digits in a code (excluding separator). Codes
// with this length have a precision of less than 1e-10 cm at the equator.
extern const size_t kMaximumDigitCount;
// Padding is used when less precise codes are desired.
extern const char kPaddingCharacter;
// The alphabet of the codes.
extern const char kAlphabet[];
// Lookup table of the alphabet positions of characters 'C' through 'X',
// inclusive. A value of -1 means the character isn't part of the alphabet.
extern const int kPositionLUT['X' - 'C' + 1];
// The number base used for the encoding.
extern const size_t kEncodingBase;
// How many characters use the pair algorithm.
extern const size_t kPairCodeLength;
// Number of columns in the grid refinement method.
extern const size_t kGridColumns;
// Number of rows in the grid refinement method.
extern const size_t kGridRows;
// Gives the exponent used for the first pair.
extern const size_t kInitialExponent;
// Size of the initial grid in degrees. This is the size of the area represented
// by a 10 character code, and is kEncodingBase ^ (2 - kPairCodeLength / 2).
extern const double kGridSizeDegrees;
}  // namespace internal

}  // namespace openlocationcode

#endif  // LOCATION_OPENLOCATIONCODE_OPENLOCATIONCODE_H_
